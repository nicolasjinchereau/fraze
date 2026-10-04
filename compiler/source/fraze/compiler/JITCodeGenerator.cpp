/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <algorithm>
#include <format>
#include <string>
#include <typeinfo>
#include <utility>
#include <fraze/compiler/Compiler.h>
#include <fraze/compiler/JITCodeGenerator.h>
#include <mir-gen.h>

namespace fraze {

void JITCodeGenerator::AppendInsn(MIR_insn_t insn) {
    MIR_append_insn(program->context, currentFunctionItem, insn);
}

static MIR_insn_code_t MoveCodeOf(MIR_type_t type) {
    return type == MIR_T_D ? MIR_DMOV : MIR_MOV;
}

// Fraze allows a shadowed local, but MIR rejects a repeated register name, so registers are named by index.
std::string JITCodeGenerator::NewRegisterName() {
    return std::format("$r{}", registerCount++);
}

MIR_reg_t JITCodeGenerator::NewRegister(MIR_type_t type)
{
    MIR_context_t context = program->context;
    MIR_func_t func = MIR_get_item_func(context, currentFunctionItem);
    return MIR_new_func_reg(context, func, type, NewRegisterName().c_str());
}

MIR_type_t JITCodeGenerator::MIRTypeOf(Type* type, const SourceLocation& loc)
{
    if(type->IsBoolean() || type->IsInteger())
        return MIR_T_I64;

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

MIR_item_t JITCodeGenerator::GetProtoItem(const sptr<FunctionDefinition>& def)
{
    MIR_context_t context = program->context;
    MIR_type_t resultType = GetResultType(def);
    std::vector<MIR_type_t> paramTypes = GetParameterTypes(def);
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
        // MIR replaces each name with its own copy, so these only have to outlive the call.
        std::vector<std::string> paramNames;
        std::vector<MIR_var_t> paramVars;

        for(size_t i = 0; i != paramTypes.size(); ++i)
        {
            paramNames.push_back(std::format("$a{}", i));
            paramVars.push_back(MIR_var_t{ paramTypes[i], paramNames.back().c_str(), 0 });
        }

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
        it->second = MIR_new_forward(program->context, GetFunctionName(def).c_str());

    return it->second;
}

void JITCodeGenerator::ThrowUnsupported(const ASTNode& node)
{
    std::string_view className = typeid(node).name();
    className = className.substr(className.rfind(':') + 1);
    Throw(node.loc, "the JIT can't generate {} yet", className);
    std::unreachable();
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

    program = spnew<JITProgram>();
    MIR_context_t context = program->context;

    MIR_module_t module = MIR_new_module(context, "program");

    try
    {
        VisitChild(node->global);
    }
    catch(...)
    {
        // MIR_finish, called by ~JITProgram, reads freed memory if a function or module is left open
        if(currentFunctionItem)
        {
            MIR_finish_func(context);
        }

        MIR_finish_module(context);
        throw;
    }

    MIR_finish_module(context);
    MIR_load_module(context, module);

    MIR_gen_init(context);
    MIR_gen_set_optimize_level(context, uint32_t(Compiler::GetActiveCompiler()->optimization));
    MIR_link(context, MIR_set_gen_interface, nullptr);
    MIR_gen_finish(context);

    // The host invokes a function by qualified name, which can't name one overload, so the first one wins.
    for(auto& [qualifiedName, item] : generatedFunctions)
    {
        program->functionAddresses.try_emplace(std::string(qualifiedName), item->addr);
    }
}

/*****************************
*         DEFINITIONS        *
*****************************/

void JITCodeGenerator::Visit(const sptr<FunctionDefinition>& node)
{
    ENFORCE(!node->isExternal, node->loc, "the JIT can't generate an extern function yet");

    MIR_context_t context = program->context;
    MIR_type_t resultType = GetResultType(node);
    std::vector<MIR_type_t> paramTypes = GetParameterTypes(node);

    registerCount = 0;
    variableRegisters.clear();

    // MIR replaces each name with its own copy, so these only have to outlive the call.
    std::vector<std::string> paramNames;
    std::vector<MIR_var_t> paramVars;

    for(MIR_type_t paramType : paramTypes)
    {
        paramNames.push_back(NewRegisterName());
        paramVars.push_back(MIR_var_t{ paramType, paramNames.back().c_str(), 0 });
    }

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
    ENFORCE(node->operation == TokenType::Assign, node->loc,
        "the JIT can't generate a compound assignment yet");

    VisitChild(node->right);
    MIR_op_t valueOp = exprValueOp;

    // A local is a register, so reading the target yields the register to assign to.
    VisitChild(node->left);
    MIR_op_t targetOp = exprValueOp;

    MIR_type_t type = MIRTypeOf(node->left->EvaluateType(), node->loc);
    AppendInsn(MIR_new_insn(program->context, MoveCodeOf(type), targetOp, valueOp));

    exprValueOp = targetOp;
}

void JITCodeGenerator::Visit(const sptr<BooleanLiteralExpression>& node) {
    exprValueOp = MIR_new_int_op(program->context, node->value ? 1 : 0);
}

void JITCodeGenerator::Visit(const sptr<CallExpression>& node)
{
    ENFORCE(node->target->EvaluateType()->IsFunction(), node->loc, "the JIT can't generate an indirect call yet");

    auto func = node->target->ToIdentifierExpression()->targetDef->ToFunctionDefinition();

    MIR_context_t context = program->context;
    MIR_type_t resultType = GetResultType(func);

    std::vector<MIR_op_t> ops {
        MIR_new_ref_op(context, GetProtoItem(func)),
        MIR_new_ref_op(context, GetFunctionItem(func))
    };

    if(resultType != MIR_T_UNDEF)
    {
        ops.push_back(MIR_new_reg_op(context, NewRegister(resultType)));
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

void JITCodeGenerator::Visit(const sptr<IdentifierExpression>& node)
{
    ENFORCE(!node->context, node->loc, "the JIT can't generate a member access yet");

    auto it = variableRegisters.find(node->targetDef);
    ENFORCE(it != variableRegisters.end(), node->loc, "the JIT can't generate a reference to '{}' yet", node->value);

    exprValueOp = MIR_new_reg_op(program->context, it->second);
}

void JITCodeGenerator::Visit(const sptr<IntegerLiteralExpression>& node) {
    exprValueOp = MIR_new_int_op(program->context, node->value);
}

void JITCodeGenerator::Visit(const sptr<NumberLiteralExpression>& node) {
    exprValueOp = MIR_new_double_op(program->context, node->value);
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

    ENFORCE(!def->isStatic, def->loc, "the JIT can't generate a static variable yet");

    MIR_type_t type = MIRTypeOf(def->typeSpec->type, def->loc);

    VisitChild(def->initializer);
    MIR_op_t valueOp = exprValueOp;

    MIR_context_t context = program->context;
    MIR_reg_t reg = NewRegister(type);
    variableRegisters[def.get()] = reg;

    AppendInsn(MIR_new_insn(context, MoveCodeOf(type), MIR_new_reg_op(context, reg), valueOp));
}

void JITCodeGenerator::Visit(const sptr<ReturnStatement>& node)
{
    if(!node->expression)
    {
        AppendInsn(MIR_new_ret_insn(program->context, 0));
        return;
    }

    VisitChild(node->expression);
    AppendInsn(MIR_new_ret_insn(program->context, 1, exprValueOp));
}

} // fraze
