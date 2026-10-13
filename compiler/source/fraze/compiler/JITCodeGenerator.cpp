/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <algorithm>
#include <cmath>
#include <format>
#include <ranges>
#include <string>
#include <typeinfo>
#include <utility>
#include <fraze/common/ScopeUtil.h>
#include <fraze/compiler/Compiler.h>
#include <fraze/compiler/JITCodeGenerator.h>
#include <fraze/compiler/RuntimeTypeInfo.h>
#include <fraze/memory/PersistentAllocator.h>
#include <mir-gen.h>

namespace fraze {

void JITCodeGenerator::AppendInsn(MIR_insn_t insn) {
    MIR_append_insn(context, currentFunctionItem, insn);
}

static MIR_insn_code_t MoveCodeOf(MIR_type_t type) {
    return type == MIR_T_D ? MIR_DMOV : MIR_MOV;
}

// MIR has no instruction for a floating-point remainder, so 'num %' calls this.
static Number JITFmod(Number left, Number right) {
    return std::fmod(left, right);
}

// Returns the code address of the receiver's implementation of an interface function. An interface call calls
// this to find its callee.
static void* JITGetInterfaceImplementationAddress(
    Program* program, Class* receiver, size_t interfaceID, size_t interfaceFuncOffset)
{
    size_t implementationFuncID = receiver->GetFunctionID(interfaceID, interfaceFuncOffset);
    return program->typeInfo[implementationFuncID]->ToFunctionInfo()->functionAddress;
}

// The MIR instruction for 'operation' on operands of 'operandType', which also accepts each operator's compound
// assignment form. Returns MIR_INVALID_INSN for an operation with no single instruction.
static MIR_insn_code_t ArithmeticCodeOf(TokenType operation, MIR_type_t operandType)
{
    bool isNumber = operandType == MIR_T_D;

    switch(operation)
    {
    case TokenType::Add: case TokenType::AddAssign: return isNumber ? MIR_DADD : MIR_ADD;
    case TokenType::Sub: case TokenType::SubAssign: return isNumber ? MIR_DSUB : MIR_SUB;
    case TokenType::Mul: case TokenType::MulAssign: return isNumber ? MIR_DMUL : MIR_MUL;
    case TokenType::Div: case TokenType::DivAssign: return isNumber ? MIR_DDIV : MIR_DIV;
    case TokenType::Mod: case TokenType::ModAssign: return isNumber ? MIR_INVALID_INSN : MIR_MOD;

    case TokenType::BitAnd: case TokenType::BitAndAssign: return MIR_AND;
    case TokenType::BitOr: case TokenType::BitOrAssign: return MIR_OR;
    case TokenType::BitXor: case TokenType::BitXorAssign: return MIR_XOR;
    case TokenType::LeftShift: case TokenType::LeftShiftAssign: return MIR_LSH;
    case TokenType::RightShift: case TokenType::RightShiftAssign: return MIR_RSH;

    case TokenType::Equal: return isNumber ? MIR_DEQ : MIR_EQ;
    case TokenType::NotEqual: return isNumber ? MIR_DNE : MIR_NE;
    case TokenType::Less: return isNumber ? MIR_DLT : MIR_LT;
    case TokenType::LessEqual: return isNumber ? MIR_DLE : MIR_LE;
    case TokenType::Greater: return isNumber ? MIR_DGT : MIR_GT;
    case TokenType::GreaterEqual: return isNumber ? MIR_DGE : MIR_GE;

    default: return MIR_INVALID_INSN;
    }
}

// A comparison yields a Fraze bool, so its result register is an integer whatever its operands are.
static bool IsComparison(TokenType operation)
{
    switch(operation)
    {
    case TokenType::Equal:
    case TokenType::NotEqual:
    case TokenType::Less:
    case TokenType::LessEqual:
    case TokenType::Greater:
    case TokenType::GreaterEqual:
    case TokenType::BitTest:
        return true;
    default:
        return false;
    }
}

// Fraze allows a shadowed local, but MIR rejects a repeated register name, so registers are named by index.
std::string JITCodeGenerator::NewRegisterName() {
    return std::format("$r{}", registerCount++);
}

// MIR has no pointer register, so a reference lives in an integer one.
MIR_reg_t JITCodeGenerator::NewRegister(MIR_type_t type)
{
    MIR_func_t func = MIR_get_item_func(context, currentFunctionItem);
    MIR_type_t registerType = type == MIR_T_P ? MIR_T_I64 : type;

    return MIR_new_func_reg(context, func, registerType, NewRegisterName().c_str());
}

// The MIR type a value of 'type' has. A struct's value is the address of its storage, so it is a pointer like a
// reference is.
MIR_type_t JITCodeGenerator::MIRTypeOf(Type* type, const SourceLocation& loc)
{
    if(type->IsBoolean() || type->IsInteger() || type->IsEnum())
        return MIR_T_I64;

    if(type->IsString() || type->IsNull() || type->IsObject() || type->IsClass() || type->IsInterface()
        || type->IsArray() || type->IsStruct())
    {
        return MIR_T_P;
    }

    ENFORCE(type->IsNumber(), loc, "the JIT can't generate the type '{}' yet", type->GetName());

    return MIR_T_D;
}

size_t JITCodeGenerator::GetStructSize(Type* structType) {
    return typeInfo.at(structType)->ToStructInfo()->size;
}

// Returns a register holding the address of storage for a struct in the function's frame, which lasts until the
// function returns. The ALLOCA goes at the start of the function, because one that ran on every pass through a
// loop would grow the stack each time.
MIR_reg_t JITCodeGenerator::NewStructStorage(Type* structType)
{
    MIR_reg_t addressReg = NewRegister(MIR_T_I64);
    MIR_op_t sizeOp = MIR_new_int_op(context, static_cast<int64_t>(GetStructSize(structType) * sizeof(Word)));

    MIR_prepend_insn(context, currentFunctionItem,
        MIR_new_insn(context, MIR_ALLOCA, MIR_new_reg_op(context, addressReg), sizeOp));

    return addressReg;
}

// Returns MIR_T_UNDEF for a function with no MIR result, which MIR never uses as a result type, so the result
// count is one for every other type. A void function has none, and neither does one that returns a struct, which
// it writes to storage the caller passes.
MIR_type_t JITCodeGenerator::GetResultType(const sptr<FunctionDefinition>& def)
{
    Type* returnType = def->returnType->type;
    return returnType->IsVoid() || returnType->IsStruct() ? MIR_T_UNDEF : MIRTypeOf(returnType, def->loc);
}

// Returns how a host that invokes 'def' gets its result, which is Unsupported when 'def' takes or returns a
// struct.
CompiledResultKind JITCodeGenerator::GetCompiledResultKind(const sptr<FunctionDefinition>& def)
{
    Type* returnType = def->returnType->type;

    bool hasStructParam = std::ranges::any_of(def->GetChildren<ParameterDefinition>(),
        [](const auto& param) { return param->typeSpec->type->IsStruct(); });

    if(hasStructParam || returnType->IsStruct())
        return CompiledResultKind::Unsupported;

    if(returnType->IsVoid())
        return CompiledResultKind::None;

    return returnType->IsNumber() ? CompiledResultKind::Number : CompiledResultKind::Integer;
}

// The address of a struct result's storage leads. A struct parameter is an address whether it is passed by value
// or by reference: by value, it is the address of a copy the caller made.
std::vector<MIR_type_t> JITCodeGenerator::GetParameterTypes(const sptr<FunctionDefinition>& def)
{
    std::vector<MIR_type_t> types;

    if(def->returnType->type->IsStruct())
        types.push_back(MIR_T_P);

    for(const auto& param : def->GetChildren<ParameterDefinition>())
    {
        ENFORCE(!param->isReference || param->typeSpec->type->IsStruct(), param->loc,
            "the JIT can't generate a reference to a '{}' yet", param->typeSpec->type->GetName());

        types.push_back(MIRTypeOf(param->typeSpec->type, param->loc));
    }

    return types;
}

MIR_item_t JITCodeGenerator::GetProtoItem(MIR_type_t resultType, const std::vector<MIR_type_t>& paramTypes)
{
    std::string_view resultName = resultType == MIR_T_UNDEF ? "void" : MIR_type_str(context, resultType);

    // A proto is a signature, so one serves every function that shares it.
    std::string name = std::format("$proto.{}(", resultName);

    for(size_t i = 0; i != paramTypes.size(); ++i)
    {
        if(i != 0)
            name += ",";

        name += MIR_type_str(context, paramTypes[i]);
    }

    name += ")";

    auto [it, inserted] = protoItems.try_emplace(name, nullptr);
    if(inserted)
    {
        // MIR replaces each name with its own copy, so these only have to outlive the call. The names are all
        // in place before the first is pointed at, because growing the vector would move them.
        std::vector<std::string> paramNames;
        std::vector<MIR_var_t> paramVars;

        for(size_t i = 0; i != paramTypes.size(); ++i)
            paramNames.push_back(std::format("$a{}", i));

        for(size_t i = 0; i != paramTypes.size(); ++i)
            paramVars.push_back(MIR_var_t{ paramTypes[i], paramNames[i].c_str(), 0 });

        size_t resultCount = resultType == MIR_T_UNDEF ? 0 : 1;

        it->second = MIR_new_proto_arr(
            context, name.c_str(), resultCount, &resultType, paramVars.size(), paramVars.data());
    }

    return it->second;
}

// MIR item names must be unique. Overloads share a qualified name, and a static function can share its signature
// with a member function, whose receiver is its first parameter, so the function's type id joins the name.
std::string JITCodeGenerator::GetFunctionName(const sptr<FunctionDefinition>& def) {
    return std::format("{}#{}", def->qualifiedName.view(), typeInfo.at(def->type)->id);
}

// A call can name a function that isn't generated yet, and MIR allows only one open function at a time, so
// the call references a forward item, which MIR resolves to the definition when the module is loaded.
MIR_item_t JITCodeGenerator::GetFunctionItem(const sptr<FunctionDefinition>& def)
{
    auto [it, inserted] = functionItems.try_emplace(def->type, nullptr);

    if(inserted)
        it->second = MIR_new_forward(context, GetFunctionName(def).c_str());

    return it->second;
}

// An extern is called through its shim and a runtime helper through its own address, which MIR_load_external binds
// to the import's name before linking.
MIR_item_t JITCodeGenerator::GetImportItem(const std::string& name, void* entryAddress)
{
    auto [it, inserted] = importedFunctions.try_emplace(name, ImportedFunction{});

    if(inserted)
    {
        it->second.item = MIR_new_import(context, name.c_str());
        it->second.entryAddress = entryAddress;
    }

    return it->second.item;
}

// A goto may come before its label, so the label insn is created on first mention and appended where it belongs.
MIR_insn_t JITCodeGenerator::GetLabelInsn(const LabelStatement* label)
{
    auto [it, inserted] = labelInsns.try_emplace(label, nullptr);

    if(inserted)
        it->second = MIR_new_label(context);

    return it->second;
}

// A memory operand's base and index are registers, while a value can also be an immediate.
MIR_reg_t JITCodeGenerator::EmitRegister(MIR_op_t valueOp)
{
    if(valueOp.mode == MIR_OP_REG)
        return valueOp.u.reg;

    MIR_reg_t reg = NewRegister(MIR_T_I64);
    AppendInsn(MIR_new_insn(context, MIR_MOV, MIR_new_reg_op(context, reg), valueOp));

    return reg;
}

// The address of a struct's storage. A local or parameter's register already holds it, and for a struct in a
// static, a field or an element it is where the memory operand points.
MIR_reg_t JITCodeGenerator::EmitAddress(MIR_op_t storageOp)
{
    if(storageOp.mode == MIR_OP_REG)
        return storageOp.u.reg;

    assert(storageOp.mode == MIR_OP_MEM);

    MIR_reg_t addressReg = NewRegister(MIR_T_I64);
    MIR_op_t addressOp = MIR_new_reg_op(context, addressReg);

    AppendInsn(MIR_new_insn(context, MIR_ADD, addressOp,
        MIR_new_reg_op(context, storageOp.u.mem.base), MIR_new_int_op(context, storageOp.u.mem.disp)));

    if(storageOp.u.mem.index != 0)
    {
        MIR_op_t offsetOp = MIR_new_reg_op(context, NewRegister(MIR_T_I64));

        AppendInsn(MIR_new_insn(context, MIR_MUL, offsetOp,
            MIR_new_reg_op(context, storageOp.u.mem.index), MIR_new_int_op(context, storageOp.u.mem.scale)));
        AppendInsn(MIR_new_insn(context, MIR_ADD, addressOp, addressOp, offsetOp));
    }

    return addressReg;
}

// Memory is loaded where it is read, so a call later in the expression can't change the value it sees. A local
// or parameter is already a register, and a struct's value is the address of its storage wherever that is.
MIR_op_t JITCodeGenerator::EmitLoad(Type* type, MIR_op_t storageOp, const SourceLocation& loc)
{
    if(type->IsStruct())
        return MIR_new_reg_op(context, EmitAddress(storageOp));

    if(storageOp.mode != MIR_OP_MEM)
        return storageOp;

    MIR_type_t valueType = MIRTypeOf(type, loc);
    MIR_op_t valueOp = MIR_new_reg_op(context, NewRegister(valueType));

    AppendInsn(MIR_new_insn(context, MoveCodeOf(valueType), valueOp, storageOp));
    return valueOp;
}

// Stores a value of 'type' in 'storageOp'. A struct is copied a word at a time from the storage its value is the
// address of, so the two are independent afterwards.
void JITCodeGenerator::EmitMove(Type* type, MIR_op_t storageOp, MIR_op_t valueOp, const SourceLocation& loc)
{
    if(!type->IsStruct())
    {
        AppendInsn(MIR_new_insn(context, MoveCodeOf(MIRTypeOf(type, loc)), storageOp, valueOp));
        return;
    }

    MIR_reg_t targetReg = EmitAddress(storageOp);
    MIR_reg_t sourceReg = EmitRegister(valueOp);
    MIR_op_t wordOp = MIR_new_reg_op(context, NewRegister(MIR_T_I64));

    for(size_t i = 0; i != GetStructSize(type); ++i)
    {
        MIR_disp_t offset = static_cast<MIR_disp_t>(i * sizeof(Word));

        AppendInsn(MIR_new_insn(context, MIR_MOV, wordOp, MIR_new_mem_op(context, MIR_T_I64, offset, sourceReg, 0, 0)));
        AppendInsn(MIR_new_insn(context, MIR_MOV, MIR_new_mem_op(context, MIR_T_I64, offset, targetReg, 0, 0), wordOp));
    }
}

// Program::globals holds every static and keeps one address for the program's life, since it is sized before any
// body is generated, so a static is memory at a fixed offset from a register holding that address.
MIR_op_t JITCodeGenerator::EmitGlobalOp(const sptr<VariableDefinition>& def, const SourceLocation& loc)
{
    MIR_type_t type = MIRTypeOf(def->typeSpec->type, loc);
    MIR_reg_t baseReg = NewRegister(MIR_T_I64);

    MIR_op_t globalsBaseOp = MIR_new_int_op(context, reinterpret_cast<int64_t>(currentProgram->globals.data()));

    AppendInsn(MIR_new_insn(context, MIR_MOV, MIR_new_reg_op(context, baseReg), globalsBaseOp));
    return MIR_new_mem_op(context, type, def->offset * sizeof(Word), baseReg, 0, 0);
}

// A field is memory at a fixed offset from its owner, at the word offset RuntimeTypeInfo gave it. An object's
// fields follow its header, and a struct's start at its address.
MIR_op_t JITCodeGenerator::EmitFieldOp(
    MIR_op_t ownerOp, Type* ownerType, const sptr<VariableDefinition>& def, const SourceLocation& loc)
{
    MIR_type_t type = MIRTypeOf(def->typeSpec->type, loc);
    size_t headerSize = ownerType->IsStruct() ? 0 : ClassDataOffset;

    return MIR_new_mem_op(context, type, (headerSize + def->offset) * sizeof(Word), EmitRegister(ownerOp), 0, 0);
}

// Stores every field of a new object or struct: from its argument when 'arguments' has one for it, and from the
// field's own initializer otherwise.
void JITCodeGenerator::EmitFieldValues(
    MIR_op_t ownerOp, Type* ownerType, const std::vector<sptr<Expression>>& arguments)
{
    auto fields = ownerType->GetDefinition()->GetChildren<VariableDefinition>(
        [](auto& field) { return !field->isStatic; });

    auto currentArg = arguments.begin();

    for(const auto& field : fields)
    {
        const sptr<Expression>& value = currentArg != arguments.end() ? *currentArg++ : field->initializer;
        MIR_op_t valueOp = EmitExpressionValue(value);

        EmitMove(field->typeSpec->type, EmitFieldOp(ownerOp, ownerType, field, value->loc), valueOp, value->loc);
    }
}

// An element is memory after the array's header, at its index scaled to the size of an element.
MIR_op_t JITCodeGenerator::EmitElementOp(const sptr<IndexExpression>& node)
{
    Type* elementType = node->target->EvaluateType()->GetElementType();
    MIR_type_t type = MIRTypeOf(elementType, node->loc);

    MIR_reg_t arrayReg = EmitRegister(EmitExpressionValue(node->target));
    MIR_reg_t indexReg = EmitRegister(EmitExpressionValue(node->arg));

    // a memory operand scales its index by a word at most, so a struct's index is counted in words first
    if(elementType->IsStruct())
    {
        MIR_reg_t wordIndexReg = NewRegister(MIR_T_I64);

        AppendInsn(MIR_new_insn(context, MIR_MUL, MIR_new_reg_op(context, wordIndexReg),
            MIR_new_reg_op(context, indexReg), MIR_new_int_op(context, GetStructSize(elementType))));

        indexReg = wordIndexReg;
    }

    return MIR_new_mem_op(context, type, ArrayDataOffset * sizeof(Word), arrayReg, indexReg, sizeof(Word));
}

// Where a variable keeps its value: a register for a local or parameter, and memory for a static or a field. A
// struct local or parameter's register holds the address of its storage.
MIR_op_t JITCodeGenerator::EmitVariableOp(const sptr<IdentifierExpression>& node)
{
    auto varDef = node->targetDef->ToVariableDefinition();

    if(node->context && Expression::IsValueExpression(node->context))
    {
        Type* contextType = node->context->EvaluateType();

        ENFORCE(varDef && (contextType->IsClass() || contextType->IsStruct()), node->loc,
            "the JIT can't generate this member access yet");

        return EmitFieldOp(EmitExpressionValue(node->context), contextType, varDef, node->loc);
    }

    if(varDef && varDef->isStatic)
        return EmitGlobalOp(varDef, node->loc);

    auto it = variableRegisters.find(node->targetDef);
    ENFORCE(it != variableRegisters.end(), node->loc, "the JIT can't generate a reference to '{}' yet", node->value);

    return MIR_new_reg_op(context, it->second);
}

MIR_op_t JITCodeGenerator::EmitExpressionValue(const sptr<Expression>& expr)
{
    VisitChildNode(expr);
    return exprValueOp;
}

MIR_op_t JITCodeGenerator::EmitArithmetic(
    TokenType operation, MIR_type_t operandType, MIR_op_t leftOp, MIR_op_t rightOp, const SourceLocation& loc)
{
    MIR_type_t resultType = IsComparison(operation) ? MIR_T_I64 : operandType;
    MIR_op_t resultOp = MIR_new_reg_op(context, NewRegister(resultType));

    // The one operator that needs a second instruction. The masked bits are compared the way the VM compares
    // them, which reads a mask of the sign bit as unset.
    if(operation == TokenType::BitTest)
    {
        MIR_op_t maskedOp = MIR_new_reg_op(context, NewRegister(operandType));
        AppendInsn(MIR_new_insn(context, MIR_AND, maskedOp, leftOp, rightOp));
        AppendInsn(MIR_new_insn(context, MIR_GT, resultOp, maskedOp, MIR_new_int_op(context, 0)));
        return resultOp;
    }

    if(operation == TokenType::Mod && operandType == MIR_T_D)
    {
        std::vector<MIR_type_t> paramTypes { MIR_T_D, MIR_T_D };

        MIR_op_t ops[] {
            MIR_new_ref_op(context, GetProtoItem(MIR_T_D, paramTypes)),
            MIR_new_ref_op(context, GetImportItem("$fmod", reinterpret_cast<void*>(&JITFmod))),
            resultOp,
            leftOp,
            rightOp
        };

        AppendInsn(MIR_new_insn_arr(context, MIR_CALL, std::size(ops), ops));
        return resultOp;
    }

    MIR_insn_code_t code = ArithmeticCodeOf(operation, operandType);

    ENFORCE(code != MIR_INVALID_INSN, loc,
        "the JIT can't generate the operator '{}' yet", Lexer::GetTokenName(operation));

    AppendInsn(MIR_new_insn(context, code, resultOp, leftOp, rightOp));
    return resultOp;
}

// Branches rather than computing both sides, because the right operand must not run when the left decides the
// result.
// bool, int and enum share one MIR type, so only a conversion to or from num is an instruction.
MIR_op_t JITCodeGenerator::EmitConversion(const sptr<Expression>& value, Type* resultType, const SourceLocation& loc)
{
    MIR_type_t sourceType = MIRTypeOf(value->EvaluateType(), loc);
    MIR_type_t targetType = MIRTypeOf(resultType, loc);
    MIR_op_t valueOp = EmitExpressionValue(value);

    if(sourceType == targetType)
        return valueOp;

    MIR_op_t resultOp = MIR_new_reg_op(context, NewRegister(targetType));

    // A bool, int or num converted from an object is unboxed. The box holds it in its only field.
    if(value->EvaluateType()->IsObject())
    {
        AppendInsn(MIR_new_insn(context, MoveCodeOf(targetType), resultOp,
            MIR_new_mem_op(context, targetType, ClassDataOffset * sizeof(Word), EmitRegister(valueOp), 0, 0)));

        return resultOp;
    }

    AppendInsn(MIR_new_insn(context, targetType == MIR_T_D ? MIR_I2D : MIR_D2I, resultOp, valueOp));
    return resultOp;
}

MIR_op_t JITCodeGenerator::EmitShortCircuit(const sptr<BinaryExpression>& node)
{
    bool isOr = node->operation == TokenType::LogicalOr;

    MIR_op_t resultOp = MIR_new_reg_op(context, NewRegister(MIR_T_I64));
    MIR_insn_t shortCircuitLabel = MIR_new_label(context);
    MIR_insn_t endLabel = MIR_new_label(context);

    EmitConditionalBranch(node->left, shortCircuitLabel, isOr);
    AppendInsn(MIR_new_insn(context, MIR_MOV, resultOp, EmitExpressionValue(node->right)));
    AppendInsn(MIR_new_insn(context, MIR_JMP, MIR_new_label_op(context, endLabel)));

    AppendInsn(shortCircuitLabel);
    AppendInsn(MIR_new_insn(context, MIR_MOV, resultOp, MIR_new_int_op(context, isOr ? 1 : 0)));

    AppendInsn(endLabel);
    return resultOp;
}

// Emits a call to JITGetInterfaceImplementationAddress, which returns an interface call's callee.
MIR_op_t JITCodeGenerator::EmitInterfaceCalleeOp(
    Type* interfaceType, const sptr<FunctionDefinition>& interfaceFuncDef, MIR_op_t receiverOp)
{
    size_t interfaceID = typeInfo.at(interfaceType)->id;
    size_t interfaceFuncOffset = typeInfo.at(interfaceFuncDef->type)->ToFunctionInfo()->offset;

    std::vector<MIR_type_t> paramTypes { MIR_T_P, MIR_T_P, MIR_T_I64, MIR_T_I64 };
    MIR_op_t calleeOp = MIR_new_reg_op(context, NewRegister(MIR_T_P));

    MIR_op_t ops[] {
        MIR_new_ref_op(context, GetProtoItem(MIR_T_P, paramTypes)),
        MIR_new_ref_op(context, GetImportItem("$getInterfaceImplementationAddress",
            reinterpret_cast<void*>(&JITGetInterfaceImplementationAddress))),
        calleeOp,
        MIR_new_int_op(context, reinterpret_cast<int64_t>(currentProgram)),
        receiverOp,
        MIR_new_int_op(context, static_cast<int64_t>(interfaceID)),
        MIR_new_int_op(context, static_cast<int64_t>(interfaceFuncOffset))
    };

    AppendInsn(MIR_new_insn_arr(context, MIR_CALL, std::size(ops), ops));
    return calleeOp;
}

void JITCodeGenerator::EmitConditionalBranch(
    const sptr<Expression>& condition, MIR_insn_t target, bool expectedValue)
{
    MIR_op_t valueOp = EmitExpressionValue(condition);

    AppendInsn(MIR_new_insn(
        context, expectedValue ? MIR_BT : MIR_BF, MIR_new_label_op(context, target), valueOp));
}

// Where an assignable expression keeps its value.
MIR_op_t JITCodeGenerator::EmitStorageOp(const sptr<Expression>& target)
{
    if(auto identExpr = target->ToIdentifierExpression())
        return EmitVariableOp(identExpr);

    if(auto indexExpr = target->ToIndexExpression())
        return EmitElementOp(indexExpr);

    ThrowUnsupported(*target);
}

// Returns the value stored, which is what an assignment or a '++' yields.
MIR_op_t JITCodeGenerator::EmitStore(const sptr<Expression>& target, MIR_op_t valueOp, const SourceLocation& loc)
{
    EmitMove(target->EvaluateType(), EmitStorageOp(target), valueOp, loc);
    return valueOp;
}

void JITCodeGenerator::ThrowUnsupported(const ASTNode& node)
{
    std::string_view className = typeid(node).name();
    className = className.substr(className.rfind(':') + 1);
    Throw(node.loc, "the JIT can't generate {} yet", className);
    std::unreachable();
}

void JITCodeGenerator::VisitChildNode(const sptr<ASTNode>& node)
{
    if(!node)
        return;

    SourceLocation enclosingLocation = std::exchange(MIRContext::errorLocation, node->loc);
    ASTVisitor::VisitChildNode(node);
    MIRContext::errorLocation = enclosingLocation;
}

/*****************************
*            ROOT            *
*****************************/

void JITCodeGenerator::Visit(const sptr<ASTRoot>& node)
{
    sptr<JITProgram> program = spnew<JITProgram>();

    auto rtti = RuntimeTypeInfo(node, Compiler::GetActiveCompiler()->types);
    typeInfo = std::move(rtti.typeInfoByType);
    program->typeInfo = std::move(rtti.allTypeInfo);
    program->globals = dynamic_array<Word>(rtti.globalSize, Word(nullptr));

    context = *program->context;
    currentProgram = program.get();

    auto finally = scope_exit([&]{ context = nullptr; currentProgram = nullptr; });

    MIR_module_t module = MIR_new_module(context, "program");

    VisitChild(node->global);

    // Check for unresolved function declarations to prevent MIR_link from freezing.
    for(auto& [functionType, forwardItem] : functionItems)
    {
        ENFORCE(forwardItem->ref_def, SourceLocation(),
            "the JIT generated a call to '{}' but never defined it", MIR_item_name(context, forwardItem));
    }

    MIR_finish_module(context);
    MIR_load_module(context, module);

    for(auto& [name, import] : importedFunctions)
    {
        MIR_load_external(context, name.c_str(), import.entryAddress);
    }

    MIR_gen_init(context);
    MIR_gen_set_optimize_level(context, uint32_t(Compiler::GetActiveCompiler()->optimization));
    MIR_link(context, MIR_set_gen_interface, nullptr);
    MIR_gen_finish(context);

    // 'call_addr' is the function's code. 'addr' is a thunk that jumps to it.
    for(auto& [functionType, functionItem] : generatedFunctionItems)
    {
        typeInfo.at(functionType)->ToFunctionInfo()->functionAddress = functionItem->u.func->call_addr;
    }

    this->program = std::move(program);
}

/*****************************
*         DEFINITIONS        *
*****************************/

void JITCodeGenerator::Visit(const sptr<ClassDefinition>& node)
{
    if(node->IsTemplateDeclaration())
        return;

    ASTVisitor::Visit(node);
}

void JITCodeGenerator::Visit(const sptr<FunctionDefinition>& node)
{
    // An extern has no body, and the call that needs it creates its import. An interface's function has no body
    // either. A template is generated per instance.
    if(node->isExternal || node->isAbstract || node->IsTemplateDeclaration())
        return;

    MIR_type_t resultType = GetResultType(node);
    std::vector<MIR_type_t> paramTypes = GetParameterTypes(node);

    registerCount = 0;
    variableRegisters.clear();
    labelInsns.clear();

    // MIR replaces each name with its own copy, so these only have to outlive the call. The names are all in
    // place before the first is pointed at, because growing the vector would move them.
    std::vector<std::string> paramNames;
    std::vector<MIR_var_t> paramVars;

    for(size_t i = 0; i != paramTypes.size(); ++i)
        paramNames.push_back(NewRegisterName());

    for(size_t i = 0; i != paramTypes.size(); ++i)
        paramVars.push_back(MIR_var_t{ paramTypes[i], paramNames[i].c_str(), 0 });

    size_t resultCount = resultType == MIR_T_UNDEF ? 0 : 1;
    currentFunctionItem = MIR_new_func_arr(
        context, GetFunctionName(node).c_str(), resultCount, &resultType, paramVars.size(), paramVars.data());

    MIR_func_t func = MIR_get_item_func(context, currentFunctionItem);
    size_t paramIndex = 0;

    resultStorageReg = node->returnType->type->IsStruct()
        ? MIR_reg(context, paramNames[paramIndex++].c_str(), func)
        : 0;

    for(const auto& param : node->GetChildren<ParameterDefinition>())
    {
        variableRegisters[param.get()] = MIR_reg(context, paramNames[paramIndex++].c_str(), func);
    }

    generatedFunctionItems[node->type] = currentFunctionItem;
    typeInfo.at(node->type)->ToFunctionInfo()->compiledResultKind = GetCompiledResultKind(node);

    VisitChild(node->body);

    MIR_finish_func(context);
    currentFunctionItem = nullptr;
}

void JITCodeGenerator::Visit(const sptr<InterfaceDefinition>& node)
{
    if(node->IsTemplateDeclaration())
        return;

    ASTVisitor::Visit(node);
}

void JITCodeGenerator::Visit(const sptr<StructDefinition>& node)
{
    if(node->IsTemplateDeclaration())
        return;

    ASTVisitor::Visit(node);
}

// A definition is not code. A field's initializer is generated by each 'new' that uses it, and a local's or a
// static's by the statement that defines it.
void JITCodeGenerator::Visit(const sptr<VariableDefinition>& node)
{
}

/*****************************
*         EXPRESSIONS        *
*****************************/

void JITCodeGenerator::Visit(const sptr<AssignExpression>& node)
{
    MIR_type_t type = MIRTypeOf(node->left->EvaluateType(), node->loc);
    MIR_op_t valueOp;

    if(node->operation == TokenType::Assign)
    {
        valueOp = EmitExpressionValue(node->right);
    }
    else
    {
        MIR_op_t leftOp = EmitExpressionValue(node->left);
        MIR_op_t rightOp = EmitExpressionValue(node->right);
        valueOp = EmitArithmetic(node->operation, type, leftOp, rightOp, node->loc);
    }

    exprValueOp = EmitStore(node->left, valueOp, node->loc);
}

void JITCodeGenerator::Visit(const sptr<BinaryExpression>& node)
{
    if(node->operation == TokenType::LogicalAnd || node->operation == TokenType::LogicalOr)
    {
        exprValueOp = EmitShortCircuit(node);
        return;
    }

    MIR_type_t operandType = MIRTypeOf(node->left->EvaluateType(), node->loc);
    MIR_op_t leftOp = EmitExpressionValue(node->left);
    MIR_op_t rightOp = EmitExpressionValue(node->right);

    exprValueOp = EmitArithmetic(node->operation, operandType, leftOp, rightOp, node->loc);
}

void JITCodeGenerator::Visit(const sptr<BooleanLiteralExpression>& node) {
    exprValueOp = MIR_new_int_op(context, node->value ? 1 : 0);
}

void JITCodeGenerator::Visit(const sptr<CallExpression>& node)
{
    Type* targetType = node->target->EvaluateType();

    ENFORCE(targetType->IsFunction() || targetType->IsFunctorInterface(), node->loc,
        "the JIT can't generate a call to a '{}' yet", targetType->GetName());

    // A call to a functor calls its 'invoke' function. The analyzer has made the functor the first argument.
    auto func = targetType->IsFunction()
        ? node->target->ToIdentifierExpression()->targetDef->ToFunctionDefinition()
        : targetType->GetDefinition()->GetFunction("invoke");

    Type* returnType = func->returnType->type;

    MIR_type_t resultType = GetResultType(func);
    std::vector<MIR_type_t> paramTypes = GetParameterTypes(func);

    // A shim takes the program ahead of everything else, whether or not the C++ function does.
    if(func->isExternal)
        paramTypes.insert(paramTypes.begin(), MIR_T_P);

    // this is the condition VMCodeGenerator uses to emit CallVirtual
    bool isInterfaceCall = func->HasImplicitThisParam() && node->arguments[0]->EvaluateType()->IsInterface();

    // an interface call's callee is set below, once its receiver has been evaluated
    std::vector<MIR_op_t> ops { MIR_new_ref_op(context, GetProtoItem(resultType, paramTypes)), MIR_op_t{} };
    size_t calleeOpIndex = 1;

    if(!isInterfaceCall)
    {
        ops[calleeOpIndex] = MIR_new_ref_op(context, func->isExternal
            ? GetImportItem(GetFunctionName(func), func->externalFunction->GetJITEntry())
            : GetFunctionItem(func));
    }

    MIR_op_t resultOp{};

    if(resultType != MIR_T_UNDEF)
    {
        resultOp = MIR_new_reg_op(context, NewRegister(resultType));
        ops.push_back(resultOp);
    }

    if(func->isExternal)
    {
        ops.push_back(MIR_new_int_op(context, reinterpret_cast<int64_t>(currentProgram)));
    }

    // a struct comes back in storage of the caller's, whose address is the call's value
    if(returnType->IsStruct())
    {
        resultOp = MIR_new_reg_op(context, NewStructStorage(returnType));
        ops.push_back(resultOp);
    }

    auto params = func->GetChildren<ParameterDefinition>();
    auto param = params.begin();
    size_t receiverOpIndex = ops.size();

    for(auto& arg : node->arguments)
    {
        Type* paramType = (*param)->typeSpec->type;
        MIR_op_t argOp = EmitExpressionValue(arg);

        // a struct passed by value is a copy, which the callee is free to change. An extern never changes an
        // argument, so it takes the caller's own struct.
        if(paramType->IsStruct() && !(*param)->isReference && !func->isExternal)
        {
            MIR_op_t copyOp = MIR_new_reg_op(context, NewStructStorage(paramType));
            EmitMove(paramType, copyOp, argOp, arg->loc);
            argOp = copyOp;
        }

        ops.push_back(argOp);
        ++param;
    }

    if(isInterfaceCall)
    {
        Type* interfaceType = node->arguments[0]->EvaluateType();
        ops[calleeOpIndex] = EmitInterfaceCalleeOp(interfaceType, func, ops[receiverOpIndex]);
    }

    AppendInsn(MIR_new_insn_arr(context, MIR_CALL, ops.size(), ops.data()));

    if(!returnType->IsVoid())
    {
        exprValueOp = resultOp;
    }
}

void JITCodeGenerator::Visit(const sptr<AsExpression>& node) {
    exprValueOp = EmitConversion(node->value, node->typeSpec->GetType(), node->loc);
}

void JITCodeGenerator::Visit(const sptr<ConvertExpression>& node) {
    exprValueOp = EmitConversion(node->value, node->resultTypeSpec->type, node->loc);
}

// Only an 'is' on a value that can't be null gets here, so the answer is known now. SemanticAnalyzer turns
// every other 'is' into a call to Type.IsInstance.
void JITCodeGenerator::Visit(const sptr<IsExpression>& node)
{
    bool isSameType = node->value->EvaluateType() == node->typeSpec->GetType();
    exprValueOp = MIR_new_int_op(context, isSameType ? 1 : 0);
}

void JITCodeGenerator::Visit(const sptr<SizeOfExpression>& node)
{
    size_t sizeInWords = typeInfo.at(node->typeSpec->type)->GetSize();
    exprValueOp = MIR_new_int_op(context, static_cast<int64_t>(sizeInWords));
}

// A cast reinterprets a reference, so the value crosses it unchanged.
void JITCodeGenerator::Visit(const sptr<CastExpression>& node) {
    exprValueOp = EmitExpressionValue(node->value);
}

void JITCodeGenerator::Visit(const sptr<CheckSiteExpression>& node)
{
    auto siteId = static_cast<Integer>(currentProgram->checkSites.size());
    currentProgram->checkSites.push_back(CheckSite{ node->message, node->loc });

    exprValueOp = MIR_new_int_op(context, siteId);
}

void JITCodeGenerator::Visit(const sptr<NullLiteralExpression>& node) {
    exprValueOp = MIR_new_int_op(context, 0);
}

// A literal is allocated persistently, so it is never collected or moved and compiled code can hold its address.
void JITCodeGenerator::Visit(const sptr<StringLiteralExpression>& node)
{
    PersistentAllocator allocator(currentProgram);
    String* value = String::New(allocator, node->value);

    exprValueOp = MIR_new_int_op(context, reinterpret_cast<int64_t>(value));
}

void JITCodeGenerator::Visit(const sptr<DefaultValueExpression>& node)
{
    Type* type = node->typeSpec->type;

    // a struct's default value gives every field its own initializer
    if(type->IsStruct())
    {
        MIR_op_t structOp = MIR_new_reg_op(context, NewStructStorage(type));
        EmitFieldValues(structOp, type, {});

        exprValueOp = structOp;
        return;
    }

    exprValueOp = MIRTypeOf(type, node->loc) == MIR_T_D ? MIR_new_double_op(context, 0.0) : MIR_new_int_op(context, 0);
}

void JITCodeGenerator::Visit(const sptr<IdentifierExpression>& node)
{
    if(auto enumMemberDef = node->targetDef->ToEnumMemberDefinition())
    {
        exprValueOp = EmitExpressionValue(enumMemberDef->value);
        return;
    }

    exprValueOp = EmitLoad(node->EvaluateType(), EmitVariableOp(node), node->loc);
}

void JITCodeGenerator::Visit(const sptr<IndexExpression>& node) {
    exprValueOp = EmitLoad(node->EvaluateType(), EmitElementOp(node), node->loc);
}

// An array's header holds its length in words, which is its count times the size of an element.
void JITCodeGenerator::Visit(const sptr<ArrayCountExpression>& node)
{
    Type* elementType = node->array->EvaluateType()->GetElementType();

    MIR_reg_t arrayReg = EmitRegister(EmitExpressionValue(node->array));
    MIR_op_t countOp = MIR_new_reg_op(context, NewRegister(MIR_T_I64));

    AppendInsn(MIR_new_insn(context, MIR_MOV, countOp,
        MIR_new_mem_op(context, MIR_T_I64, ArrayLengthOffset * sizeof(Word), arrayReg, 0, 0)));

    if(elementType->IsStruct())
    {
        AppendInsn(MIR_new_insn(context, MIR_DIV, countOp, countOp,
            MIR_new_int_op(context, GetStructSize(elementType))));
    }

    exprValueOp = countOp;
}

// The value of a fold is its last statement's expression, which is visited as an expression so that its value
// isn't discarded the way a statement's is.
void JITCodeGenerator::Visit(const sptr<FoldExpression>& node)
{
    auto& statements = node->body->statements;

    for(size_t i = 0; i != statements.size() - 1; ++i)
        VisitChild(statements[i]);

    exprValueOp = EmitExpressionValue(node->GetResultStatement()->expression);
}

void JITCodeGenerator::Visit(const sptr<IntegerLiteralExpression>& node) {
    exprValueOp = MIR_new_int_op(context, node->value);
}

// The analyzer leaves an allocation in 'allocExpression', as a call to Type.NewClass or Type.NewArray, while a
// struct is built in this function's frame. An array then stores the elements it lists, and a class or struct
// every one of its fields.
void JITCodeGenerator::Visit(const sptr<NewExpression>& node)
{
    Type* type = node->EvaluateType();

    if(type->IsArray())
    {
        Type* elementType = type->GetElementType();
        MIR_type_t elementMIRType = MIRTypeOf(elementType, node->loc);
        size_t elementSize = elementType->IsStruct() ? GetStructSize(elementType) : 1;

        MIR_reg_t arrayReg = EmitRegister(EmitExpressionValue(node->allocExpression));

        for(size_t i = 0; i != node->arguments.size(); ++i)
        {
            MIR_op_t valueOp = EmitExpressionValue(node->arguments[i]);
            MIR_op_t elementOp = MIR_new_mem_op(
                context, elementMIRType, (ArrayDataOffset + i * elementSize) * sizeof(Word), arrayReg, 0, 0);

            EmitMove(elementType, elementOp, valueOp, node->arguments[i]->loc);
        }

        exprValueOp = MIR_new_reg_op(context, arrayReg);
        return;
    }

    ENFORCE(type->IsClass() || type->IsStruct(), node->loc, "the JIT can't generate a new '{}' yet", type->GetName());

    MIR_op_t ownerOp = type->IsStruct()
        ? MIR_new_reg_op(context, NewStructStorage(type))
        : EmitExpressionValue(node->allocExpression);

    EmitFieldValues(ownerOp, type, node->arguments);
    exprValueOp = ownerOp;
}

void JITCodeGenerator::Visit(const sptr<NumberLiteralExpression>& node) {
    exprValueOp = MIR_new_double_op(context, node->value);
}

void JITCodeGenerator::Visit(const sptr<PostfixExpression>& node)
{
    ENFORCE(node->operation == TokenType::Increment || node->operation == TokenType::Decrement, node->loc,
        "the JIT can't generate the operator '{}' yet", Lexer::GetTokenName(node->operation));

    MIR_type_t type = MIRTypeOf(node->arg->EvaluateType(), node->loc);
    MIR_op_t argOp = EmitExpressionValue(node->arg);

    // The result is the value from before the update, so it outlives the store into the target.
    MIR_op_t previousValueOp = MIR_new_reg_op(context, NewRegister(type));
    AppendInsn(MIR_new_insn(context, MoveCodeOf(type), previousValueOp, argOp));

    TokenType operation = node->operation == TokenType::Increment ? TokenType::Add : TokenType::Sub;
    MIR_op_t oneOp = type == MIR_T_D ? MIR_new_double_op(context, 1.0) : MIR_new_int_op(context, 1);
    MIR_op_t valueOp = EmitArithmetic(operation, type, previousValueOp, oneOp, node->loc);

    EmitStore(node->arg, valueOp, node->loc);
    exprValueOp = previousValueOp;
}

void JITCodeGenerator::Visit(const sptr<PrefixExpression>& node)
{
    MIR_type_t type = MIRTypeOf(node->arg->EvaluateType(), node->loc);

    switch(node->operation)
    {
    case TokenType::Add:
        exprValueOp = EmitExpressionValue(node->arg);
        break;

    case TokenType::Sub:
    {
        MIR_op_t argOp = EmitExpressionValue(node->arg);
        MIR_op_t resultOp = MIR_new_reg_op(context, NewRegister(type));
        AppendInsn(MIR_new_insn(context, type == MIR_T_D ? MIR_DNEG : MIR_NEG, resultOp, argOp));
        exprValueOp = resultOp;
        break;
    }
    case TokenType::BitNot:
    {
        MIR_op_t argOp = EmitExpressionValue(node->arg);
        MIR_op_t resultOp = MIR_new_reg_op(context, NewRegister(type));
        AppendInsn(MIR_new_insn(context, MIR_XOR, resultOp, argOp, MIR_new_int_op(context, -1)));
        exprValueOp = resultOp;
        break;
    }
    case TokenType::LogicalNot:
    {
        MIR_op_t argOp = EmitExpressionValue(node->arg);
        MIR_op_t resultOp = MIR_new_reg_op(context, NewRegister(MIR_T_I64));
        AppendInsn(MIR_new_insn(context, MIR_EQ, resultOp, argOp, MIR_new_int_op(context, 0)));
        exprValueOp = resultOp;
        break;
    }
    case TokenType::Increment:
    case TokenType::Decrement:
    {
        TokenType operation = node->operation == TokenType::Increment ? TokenType::Add : TokenType::Sub;
        MIR_op_t argOp = EmitExpressionValue(node->arg);
        MIR_op_t oneOp = type == MIR_T_D ? MIR_new_double_op(context, 1.0) : MIR_new_int_op(context, 1);
        MIR_op_t valueOp = EmitArithmetic(operation, type, argOp, oneOp, node->loc);
        exprValueOp = EmitStore(node->arg, valueOp, node->loc);
        break;
    }
    default:
        ENFORCE(false, node->loc,
            "the JIT can't generate the operator '{}' yet", Lexer::GetTokenName(node->operation));
    }
}

void JITCodeGenerator::Visit(const sptr<TernaryExpression>& node)
{
    MIR_type_t type = MIRTypeOf(node->EvaluateType(), node->loc);
    MIR_op_t resultOp = MIR_new_reg_op(context, NewRegister(type));
    MIR_insn_t falseLabel = MIR_new_label(context);
    MIR_insn_t endLabel = MIR_new_label(context);

    EmitConditionalBranch(node->condition, falseLabel, false);

    MIR_op_t trueValueOp = EmitExpressionValue(node->trueValue);
    AppendInsn(MIR_new_insn(context, MoveCodeOf(type), resultOp, trueValueOp));
    AppendInsn(MIR_new_insn(context, MIR_JMP, MIR_new_label_op(context, endLabel)));

    AppendInsn(falseLabel);
    MIR_op_t falseValueOp = EmitExpressionValue(node->falseValue);
    AppendInsn(MIR_new_insn(context, MoveCodeOf(type), resultOp, falseValueOp));

    AppendInsn(endLabel);
    exprValueOp = resultOp;
}

// A type's id, which is its index in Program::typeInfo.
void JITCodeGenerator::Visit(const sptr<TypeLiteralExpression>& node)
{
    TypeInfo* type = currentProgram->GetTypeInfo(node->value);
    exprValueOp = MIR_new_int_op(context, static_cast<int64_t>(type->id));
}

/*****************************
*         STATEMENTS         *
*****************************/

void JITCodeGenerator::Visit(const sptr<BlockStatement>& node)
{
    for(auto& stmt : node->statements)
        VisitChild(stmt);
}

void JITCodeGenerator::Visit(const sptr<ExpressionStatement>& node) {
    VisitChild(node->expression);
}

void JITCodeGenerator::Visit(const sptr<VariableDefinitionStatement>& node)
{
    auto& def = node->variableDefinition;
    Type* type = def->typeSpec->type;
    MIR_op_t valueOp = EmitExpressionValue(def->initializer);

    if(def->isStatic)
    {
        EmitMove(type, EmitGlobalOp(def, def->loc), valueOp, def->loc);
        return;
    }

    // a struct local is storage in the frame, and its register holds the address of it
    MIR_reg_t reg = type->IsStruct() ? NewStructStorage(type) : NewRegister(MIRTypeOf(type, def->loc));
    variableRegisters[def.get()] = reg;

    EmitMove(type, MIR_new_reg_op(context, reg), valueOp, def->loc);
}

void JITCodeGenerator::Visit(const sptr<ReturnStatement>& node)
{
    if(!node->expression)
    {
        AppendInsn(MIR_new_ret_insn(context, 0));
        return;
    }

    MIR_op_t valueOp = EmitExpressionValue(node->expression);

    // a struct goes into the storage the caller passed for it
    if(resultStorageReg != 0)
    {
        EmitMove(node->expression->EvaluateType(), MIR_new_reg_op(context, resultStorageReg), valueOp, node->loc);
        AppendInsn(MIR_new_ret_insn(context, 0));
        return;
    }

    AppendInsn(MIR_new_ret_insn(context, 1, valueOp));
}


void JITCodeGenerator::Visit(const sptr<EmptyStatement>& node) {
    ASTVisitor::Visit(node);
}

void JITCodeGenerator::Visit(const sptr<ForStatement>& node)
{
    MIR_insn_t conditionLabel = MIR_new_label(context);
    MIR_insn_t endLabel = MIR_new_label(context);

    VisitChild(node->init);
    AppendInsn(conditionLabel);

    // without a condition, the loop never exits through the top
    if(node->condition)
        EmitConditionalBranch(node->condition, endLabel, false);

    VisitChild(node->body);
    VisitChild(node->iterate);
    AppendInsn(MIR_new_insn(context, MIR_JMP, MIR_new_label_op(context, conditionLabel)));

    AppendInsn(endLabel);
}

void JITCodeGenerator::Visit(const sptr<GotoStatement>& node)
{
    AppendInsn(MIR_new_insn(context, MIR_JMP, MIR_new_label_op(context, GetLabelInsn(node->label.get()))));
}

void JITCodeGenerator::Visit(const sptr<IfStatement>& node)
{
    MIR_insn_t falseLabel = MIR_new_label(context);

    EmitConditionalBranch(node->condition, falseLabel, false);
    VisitChild(node->trueBranch);

    if(!node->falseBranch)
    {
        AppendInsn(falseLabel);
        return;
    }

    MIR_insn_t endLabel = MIR_new_label(context);
    AppendInsn(MIR_new_insn(context, MIR_JMP, MIR_new_label_op(context, endLabel)));

    AppendInsn(falseLabel);
    VisitChild(node->falseBranch);

    AppendInsn(endLabel);
}

void JITCodeGenerator::Visit(const sptr<LabelStatement>& node) {
    AppendInsn(GetLabelInsn(node.get()));
}

void JITCodeGenerator::Visit(const sptr<SwitchStatement>& node)
{
    auto& sections = node->sections;

    // each case's value and the index of its section, where one past the last section is the end
    std::vector<std::pair<int64_t, size_t>> cases;
    size_t defaultSectionIndex = sections.size();

    for(size_t i = 0; i != sections.size(); ++i)
    {
        for(auto& caseValue : sections[i].caseValues)
            cases.emplace_back(*SwitchStatement::GetCaseValue(caseValue), i);

        if(sections[i].isDefault)
            defaultSectionIndex = i;
    }

    MIR_insn_t endLabel = MIR_new_label(context);
    std::vector<MIR_insn_t> sectionLabels;

    for(size_t i = 0; i != sections.size(); ++i)
        sectionLabels.push_back(MIR_new_label(context));

    MIR_insn_t defaultLabel = defaultSectionIndex != sections.size() ? sectionLabels[defaultSectionIndex] : endLabel;

    // a table is used when at least half its entries are cases, the rest going to the default
    int64_t minCase = 0;
    uint64_t tableSize = 0;

    if(!cases.empty())
    {
        auto [min, max] = std::ranges::minmax(cases | std::views::keys);
        uint64_t caseSpan = static_cast<uint64_t>(max) - static_cast<uint64_t>(min);

        if(caseSpan < 2 * cases.size())
        {
            minCase = min;
            tableSize = caseSpan + 1;
        }
    }

    MIR_op_t valueOp = EmitExpressionValue(node->value);

    if(tableSize != 0)
    {
        MIR_op_t entryOp = MIR_new_reg_op(context, NewRegister(MIR_T_I64));
        AppendInsn(MIR_new_insn(context, MIR_SUB, entryOp, valueOp, MIR_new_int_op(context, minCase)));

        // MIR_SWITCH has no entry outside the table, and an unsigned comparison catches a value below it too
        AppendInsn(MIR_new_insn(context, MIR_UBGE, MIR_new_label_op(context, defaultLabel),
            entryOp, MIR_new_int_op(context, static_cast<int64_t>(tableSize))));

        std::vector<MIR_op_t> ops(tableSize + 1, MIR_new_label_op(context, defaultLabel));
        ops[0] = entryOp;

        for(auto [value, sectionIndex] : cases)
        {
            uint64_t entry = static_cast<uint64_t>(value) - static_cast<uint64_t>(minCase);
            ops[1 + entry] = MIR_new_label_op(context, sectionLabels[sectionIndex]);
        }

        AppendInsn(MIR_new_insn_arr(context, MIR_SWITCH, ops.size(), ops.data()));
    }
    else
    {
        for(auto [value, sectionIndex] : cases)
        {
            AppendInsn(MIR_new_insn(context, MIR_BEQ,
                MIR_new_label_op(context, sectionLabels[sectionIndex]), valueOp, MIR_new_int_op(context, value)));
        }

        AppendInsn(MIR_new_insn(context, MIR_JMP, MIR_new_label_op(context, defaultLabel)));
    }

    for(size_t i = 0; i != sections.size(); ++i)
    {
        AppendInsn(sectionLabels[i]);
        VisitChild(sections[i].body);

        // sections don't fall through, so every one but the last jumps to the end
        if(i + 1 != sections.size())
            AppendInsn(MIR_new_insn(context, MIR_JMP, MIR_new_label_op(context, endLabel)));
    }

    AppendInsn(endLabel);
}

void JITCodeGenerator::Visit(const sptr<WhileStatement>& node)
{
    MIR_insn_t conditionLabel = MIR_new_label(context);
    MIR_insn_t endLabel = MIR_new_label(context);

    AppendInsn(conditionLabel);

    // without a condition, the loop never exits through the top
    if(node->condition)
        EmitConditionalBranch(node->condition, endLabel, false);

    VisitChild(node->body);
    AppendInsn(MIR_new_insn(context, MIR_JMP, MIR_new_label_op(context, conditionLabel)));

    AppendInsn(endLabel);
}

} // fraze
