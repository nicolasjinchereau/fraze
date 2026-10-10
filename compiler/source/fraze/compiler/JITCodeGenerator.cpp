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

MIR_type_t JITCodeGenerator::MIRTypeOf(Type* type, const SourceLocation& loc)
{
    if(type->IsBoolean() || type->IsInteger() || type->IsEnum())
        return MIR_T_I64;

    if(type->IsString() || type->IsNull())
        return MIR_T_P;

    ENFORCE(type->IsNumber(), loc, "the JIT can't generate the type '{}' yet", type->GetName());

    return MIR_T_D;
}

// Returns MIR_T_UNDEF for a void function, which MIR never uses as a result type, so the result count is
// one for every other type.
MIR_type_t JITCodeGenerator::GetResultType(const sptr<FunctionDefinition>& def)
{
    Type* returnType = def->returnType->type;
    return returnType->IsVoid() ? MIR_T_UNDEF : MIRTypeOf(returnType, def->loc);
}

std::vector<MIR_type_t> JITCodeGenerator::GetParameterTypes(const sptr<FunctionDefinition>& def)
{
    std::vector<MIR_type_t> types;

    for(const auto& param : def->GetChildren<ParameterDefinition>())
    {
        ENFORCE(!param->isReference, param->loc, "the JIT can't generate a ref parameter yet");
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

// MIR item names must be unique, so the signature joins the qualified name the way an extern's lookup key does.
std::string JITCodeGenerator::GetFunctionName(const sptr<FunctionDefinition>& def) {
    return std::format("{}:{}", def->qualifiedName.view(), def->GetSignature());
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

// Where a variable keeps its value: a register for a local or parameter, and memory for a static.
MIR_op_t JITCodeGenerator::EmitVariableOp(const sptr<IdentifierExpression>& node)
{
    ENFORCE(!node->context || !Expression::IsValueExpression(node->context), node->loc,
        "the JIT can't generate a member access yet");

    if(auto varDef = node->targetDef->ToVariableDefinition(); varDef && varDef->isStatic)
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

void JITCodeGenerator::EmitConditionalBranch(
    const sptr<Expression>& condition, MIR_insn_t target, bool expectedValue)
{
    MIR_op_t valueOp = EmitExpressionValue(condition);

    AppendInsn(MIR_new_insn(
        context, expectedValue ? MIR_BT : MIR_BF, MIR_new_label_op(context, target), valueOp));
}

// Returns the value stored, which is what an assignment or a '++' yields.
MIR_op_t JITCodeGenerator::EmitStore(const sptr<Expression>& target, MIR_op_t valueOp, const SourceLocation& loc)
{
    auto identExpr = target->ToIdentifierExpression();

    if(!identExpr)
        ThrowUnsupported(*target);

    MIR_type_t type = MIRTypeOf(target->EvaluateType(), loc);

    AppendInsn(MIR_new_insn(context, MoveCodeOf(type), EmitVariableOp(identExpr), valueOp));
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
    sptr<FunctionDefinition> mainDef;
    if(auto def = node->global->scope->FindDefinition("main"))
    {
        mainDef = def->ToFunctionDefinition();
    }

    ENFORCE(mainDef, SourceLocation(), "the JIT requires a global function named 'main'");

    sptr<JITProgram> program = spnew<JITProgram>();

    auto rtti = RuntimeTypeInfo(node, Compiler::GetActiveCompiler()->types);
    typeInfo = std::move(rtti.typeInfoByType);
    program->typeInfo = std::move(rtti.allTypeInfo);
    program->globals = dynamic_array<Word>(rtti.globalSize);
    std::ranges::fill(program->globals, Word(nullptr));

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

    // Functions are mapped by qualified name, so Program::Invoke calls the first one found.
    for(auto& [qualifiedName, item] : generatedFunctions)
    {
        program->functionAddresses.try_emplace(std::string(qualifiedName), item->addr);
    }

    this->program = std::move(program);
}

/*****************************
*         DEFINITIONS        *
*****************************/

void JITCodeGenerator::Visit(const sptr<FunctionDefinition>& node)
{
    // An extern has no body, and the call that needs it creates its import.
    if(node->isExternal)
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

    for(const auto& param : node->GetChildren<ParameterDefinition>())
    {
        variableRegisters[param.get()] = MIR_reg(context, paramNames[paramIndex++].c_str(), func);
    }

    generatedFunctions.emplace_back(node->qualifiedName.view(), currentFunctionItem);

    VisitChild(node->body);

    MIR_finish_func(context);
    currentFunctionItem = nullptr;
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
    ENFORCE(node->target->EvaluateType()->IsFunction(), node->loc, "the JIT can't generate an indirect call yet");

    auto func = node->target->ToIdentifierExpression()->targetDef->ToFunctionDefinition();

    MIR_type_t resultType = GetResultType(func);
    std::vector<MIR_type_t> paramTypes = GetParameterTypes(func);

    // A shim takes the program ahead of the Fraze arguments, whether or not the C++ function does.
    if(func->isExternal)
        paramTypes.insert(paramTypes.begin(), MIR_T_P);

    std::vector<MIR_op_t> ops {
        MIR_new_ref_op(context, GetProtoItem(resultType, paramTypes)),
        MIR_new_ref_op(context, func->isExternal
            ? GetImportItem(GetFunctionName(func), func->externalFunction->GetJITEntry())
            : GetFunctionItem(func))
    };

    if(resultType != MIR_T_UNDEF)
    {
        ops.push_back(MIR_new_reg_op(context, NewRegister(resultType)));
    }

    if(func->isExternal)
    {
        ops.push_back(MIR_new_int_op(context, reinterpret_cast<int64_t>(currentProgram)));
    }

    for(auto& arg : node->arguments)
    {
        VisitChild(arg);
        ops.push_back(exprValueOp);
    }

    AppendInsn(MIR_new_insn_arr(context, MIR_CALL, ops.size(), ops.data()));

    if(resultType != MIR_T_UNDEF)
    {
        exprValueOp = ops[2];
    }
}

void JITCodeGenerator::Visit(const sptr<AsExpression>& node) {
    exprValueOp = EmitConversion(node->value, node->typeSpec->GetType(), node->loc);
}

void JITCodeGenerator::Visit(const sptr<ConvertExpression>& node) {
    exprValueOp = EmitConversion(node->value, node->resultTypeSpec->type, node->loc);
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
    MIR_type_t type = MIRTypeOf(node->typeSpec->type, node->loc);
    exprValueOp = type == MIR_T_D ? MIR_new_double_op(context, 0.0) : MIR_new_int_op(context, 0);
}

void JITCodeGenerator::Visit(const sptr<IdentifierExpression>& node)
{
    if(auto enumMemberDef = node->targetDef->ToEnumMemberDefinition())
    {
        exprValueOp = EmitExpressionValue(enumMemberDef->value);
        return;
    }

    MIR_op_t storageOp = EmitVariableOp(node);

    // A static lives in memory and is loaded where it is read, so a call later in the expression can't change
    // the value it sees. A local or parameter is already a register.
    if(storageOp.mode != MIR_OP_MEM)
    {
        exprValueOp = storageOp;
        return;
    }

    MIR_type_t type = MIRTypeOf(node->EvaluateType(), node->loc);
    MIR_op_t valueOp = MIR_new_reg_op(context, NewRegister(type));

    AppendInsn(MIR_new_insn(context, MoveCodeOf(type), valueOp, storageOp));
    exprValueOp = valueOp;
}

void JITCodeGenerator::Visit(const sptr<IntegerLiteralExpression>& node) {
    exprValueOp = MIR_new_int_op(context, node->value);
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
    MIR_type_t type = MIRTypeOf(def->typeSpec->type, def->loc);
    MIR_op_t valueOp = EmitExpressionValue(def->initializer);

    if(def->isStatic)
    {
        AppendInsn(MIR_new_insn(context, MoveCodeOf(type), EmitGlobalOp(def, def->loc), valueOp));
        return;
    }

    MIR_reg_t reg = NewRegister(type);
    variableRegisters[def.get()] = reg;

    AppendInsn(MIR_new_insn(context, MoveCodeOf(type), MIR_new_reg_op(context, reg), valueOp));
}

void JITCodeGenerator::Visit(const sptr<ReturnStatement>& node)
{
    if(!node->expression)
    {
        AppendInsn(MIR_new_ret_insn(context, 0));
        return;
    }

    VisitChild(node->expression);
    AppendInsn(MIR_new_ret_insn(context, 1, exprValueOp));
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
