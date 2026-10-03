/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <algorithm>
#include <string>
#include <typeinfo>
#include <utility>
#include <fraze/compiler/JITCodeGenerator.h>
#include <mir-gen.h>

namespace fraze {

void JITCodeGenerator::AppendInsn(MIR_insn_t insn) {
    MIR_append_insn(program->context, currentFunctionItem, insn);
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
        mainDef = def->ToFunctionDefinition();

    ENFORCE(mainDef, SourceLocation(), "the JIT requires a global function named 'main'");

    program = spnew<JITProgram>();
    MIR_context_t context = program->context;

    MIR_module_t module = MIR_new_module(context, "program");

    try
    {
        VisitChild(mainDef);
    }
    catch(...)
    {
        // MIR_finish, called by ~JITProgram, reads freed memory if a function or module is left open
        if(currentFunctionItem)
            MIR_finish_func(context);

        MIR_finish_module(context);
        throw;
    }

    MIR_finish_module(context);
    MIR_load_module(context, module);

    MIR_gen_init(context);
    MIR_link(context, MIR_set_gen_interface, nullptr);
    MIR_gen_finish(context);

    for(MIR_item_t item = DLIST_HEAD(MIR_item_t, module->items); item; item = DLIST_NEXT(MIR_item_t, item))
    {
        if(item->item_type == MIR_func_item)
            program->functionAddresses[MIR_item_name(context, item)] = item->addr;
    }
}

/*****************************
*         DEFINITIONS        *
*****************************/

void JITCodeGenerator::Visit(const sptr<FunctionDefinition>& node)
{
    bool hasParameters = std::ranges::any_of(node->scope->definitions, [](auto& def) {
        return def->ToParameterDefinition() != nullptr;
    });

    ENFORCE(!hasParameters && node->returnType->type->IsInteger(), node->loc,
        "the JIT can only generate a function that takes no parameters and returns int");

    std::string name(node->name);
    MIR_type_t resultType = MIR_T_I64;
    currentFunctionItem = MIR_new_func_arr(program->context, name.c_str(), 1, &resultType, 0, nullptr);

    VisitChild(node->body);

    MIR_finish_func(program->context);
    currentFunctionItem = nullptr;
}

/*****************************
*         EXPRESSIONS        *
*****************************/

void JITCodeGenerator::Visit(const sptr<IntegerLiteralExpression>& node) {
    exprValueOp = MIR_new_int_op(program->context, node->value);
}

/*****************************
*         STATEMENTS         *
*****************************/

void JITCodeGenerator::Visit(const sptr<BlockStatement>& node)
{
    for(auto& stmt : node->statements)
        VisitChild(stmt);
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
