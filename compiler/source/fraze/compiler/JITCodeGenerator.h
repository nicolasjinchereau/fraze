/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/AST.h>
#include <fraze/ast/ASTVisitor.h>
#include <fraze/program/JITProgram.h>
#include <mir.h>

namespace fraze {

class JITCodeGenerator : public ASTVisitor
{
    MIR_item_t currentFunctionItem{};
    MIR_op_t exprValueOp{};

    void AppendInsn(MIR_insn_t insn);
    [[noreturn]] void ThrowUnsupported(const ASTNode& node);

public:
    sptr<JITProgram> program;

    virtual void Visit(const sptr<ASTRoot>& node) override;
    virtual void Visit(const sptr<FunctionDefinition>& node) override;
    virtual void Visit(const sptr<IntegerLiteralExpression>& node) override;
    virtual void Visit(const sptr<BlockStatement>& node) override;
    virtual void Visit(const sptr<ReturnStatement>& node) override;

    virtual void Visit(const sptr<ArrayCountExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<AsExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<AssignExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<AwaitExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<BinaryExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<BooleanLiteralExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<CastExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<CallExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<CheckSiteExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<ConvertExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<DefaultValueExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<FoldExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<IdentifierExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<IndexExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<IsExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<NewExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<NullLiteralExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<NumberLiteralExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<PostfixExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<PrefixExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<SizeOfExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<StringLiteralExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<TernaryExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<TypeLiteralExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<TypeOfExpression>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<AssertStatement>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<EmptyStatement>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<ExposeStatement>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<ExpressionStatement>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<ForStatement>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<GotoStatement>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<IfStatement>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<LabelStatement>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<SwitchStatement>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<VariableDefinitionStatement>& node) override { ThrowUnsupported(*node); }
    virtual void Visit(const sptr<WhileStatement>& node) override { ThrowUnsupported(*node); }
};

} // fraze
