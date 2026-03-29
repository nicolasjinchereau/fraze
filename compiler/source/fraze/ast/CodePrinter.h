/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <sstream>
#include <print>
#include <fraze/ast/ASTVisitor.h>

namespace fraze {

class CodePrinter : public ASTVisitor
{
    std::stringstream& stream;
    int tabWidth{};
    int indent{};

    std::string GetIndent() const;
public:
    CodePrinter(std::stringstream& stream, int tabWidth = 4, int startingIndent = 0)
        : stream(stream)
        , tabWidth(tabWidth)
        , indent(startingIndent)
    {
    }

    static void Print(const sptr<ASTNode>& node) {
        std::stringstream stream;
        fraze::CodePrinter printer(stream, 2);
        printer.VisitChildNode(node);
        std::println("{}", stream.str());
    }

    virtual void Visit(const sptr<ASTRoot>& node) override;

    virtual void Visit(const sptr<BasicTypeDefinition>& node) override;
    virtual void Visit(const sptr<ClassDefinition>& node) override;
    virtual void Visit(const sptr<EnumDefinition>& node) override;
    virtual void Visit(const sptr<EnumMemberDefinition>& node) override;
    virtual void Visit(const sptr<FunctionDefinition>& node) override;
    virtual void Visit(const sptr<InterfaceDefinition>& node) override;
    virtual void Visit(const sptr<ParameterDefinition>& node) override;
    virtual void Visit(const sptr<PropertyDefinition>& node) override;
    virtual void Visit(const sptr<SectionDefinition>& node) override;
    virtual void Visit(const sptr<StructDefinition>& node) override;
    virtual void Visit(const sptr<TemplateDefinition>& node) override;
    virtual void Visit(const sptr<TemplateParameterDefinition>& node) override;
    virtual void Visit(const sptr<VariableDefinition>& node) override;

    virtual void Visit(const sptr<AsExpression>& node) override;
    virtual void Visit(const sptr<AssignExpression>& node) override;
    virtual void Visit(const sptr<AwaitExpression>& node) override;
    virtual void Visit(const sptr<BinaryExpression>& node) override;
    virtual void Visit(const sptr<BooleanLiteralExpression>& node) override;
    virtual void Visit(const sptr<CachedExpression>& node) override;
    virtual void Visit(const sptr<CastExpression>& node) override;
    virtual void Visit(const sptr<CallExpression>& node) override;
    virtual void Visit(const sptr<ConvertExpression>& node) override;
    virtual void Visit(const sptr<DefaultValueExpression>& node) override;
    virtual void Visit(const sptr<EmitExpression>& node) override;
    virtual void Visit(const sptr<FoldExpression>& node) override;
    virtual void Visit(const sptr<IdentifierExpression>& node) override;
    virtual void Visit(const sptr<IndexExpression>& node) override;
    virtual void Visit(const sptr<IntegerLiteralExpression>& node) override;
    virtual void Visit(const sptr<IsExpression>& node) override;
    virtual void Visit(const sptr<NewExpression>& node) override;
    virtual void Visit(const sptr<NullLiteralExpression>& node) override;
    virtual void Visit(const sptr<NumberLiteralExpression>& node) override;
    virtual void Visit(const sptr<PostfixExpression>& node) override;
    virtual void Visit(const sptr<PrefixExpression>& node) override;
    virtual void Visit(const sptr<SizeOfExpression>& node) override;
    virtual void Visit(const sptr<StringLiteralExpression>& node) override;
    virtual void Visit(const sptr<TernaryExpression>& node) override;
    virtual void Visit(const sptr<TypeLiteralExpression>& node) override;
    virtual void Visit(const sptr<TypeOfExpression>& node) override;

    virtual void Visit(const sptr<TypeSpecifier>& node) override;

    virtual void Visit(const sptr<AssertStatement>& node) override;
    virtual void Visit(const sptr<BlockStatement>& node) override;
    virtual void Visit(const sptr<EmptyStatement>& node) override;
    virtual void Visit(const sptr<ExposeStatement>& node) override;
    virtual void Visit(const sptr<ExpressionStatement>& node) override;
    virtual void Visit(const sptr<ForStatement>& node) override;
    virtual void Visit(const sptr<GotoStatement>& node) override;
    virtual void Visit(const sptr<IfStatement>& node) override;
    virtual void Visit(const sptr<ReturnStatement>& node) override;
    virtual void Visit(const sptr<VariableDefinitionStatement>& node) override;
    virtual void Visit(const sptr<WhileStatement>& node) override;
};

} // fraze
