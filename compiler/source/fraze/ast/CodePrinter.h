/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <optional>
#include <ostream>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <fraze/ast/ASTVisitor.h>
#include <fraze/compiler/Lexer.h>

namespace fraze {

class Scope;

// Prints the AST as Fraze-like code, true to its lowered form after semantic analysis.
// Nodes with no source syntax use pseudo-forms (cast<T>(x), default(T), typeid("T"), etc.),
// and state with no keyword is printed as attributes named after its C++ field
// ([isCoroutine], [originalClassType: App], etc.). The output is meant for reading while
// debugging the compiler; it isn't expected to compile.
class CodePrinter : public ASTVisitor
{
    // binding strength of each expression level, loosest first, following Parser's descent order
    enum class Precedence
    {
        Assignment,
        Ternary,
        LogicalOr,
        LogicalAnd,
        Equality,
        Comparison,
        BitOr,
        BitXor,
        BitAnd,
        Shift,
        AddSub,
        MulDivMod,
        Await,
        Prefix,
        Postfix,
        Primary,
    };

    std::ostream& stream;
    int tabWidth{};
    int indent{};

    // Only a section's definitions and statements located in this file are printed; the sections
    // containing them are reopened in each file, like in source. Everything is printed if unset.
    std::optional<std::string_view> printedFile;

    void PrintIndent();
    void PrintDefinitions(Scope* scope, bool skipVariables, bool hasPrecedingContent);
    bool IsInPrintedFile(const SourceLocation& loc) const;
    bool HasContentInPrintedFile(const sptr<Definition>& def) const;
    void PrintTemplateParameters(const sptr<TemplateDefinition>& node);
    void PrintAttributes(const std::vector<std::string>& attributes, bool ownLine);
    void PrintVariable(const sptr<VariableDefinition>& node, bool qualifyName, bool printInitializer);
    void PrintBlock(const sptr<BlockStatement>& node, bool endLine);
    void PrintBody(const sptr<Statement>& node);
    void PrintIfStatement(const sptr<IfStatement>& node);
    void PrintInlineStatement(const sptr<Statement>& node);
    void PrintExpression(const sptr<Expression>& expr, Precedence minPrecedence);
    void PrintArguments(const std::vector<sptr<Expression>>& args);
    void PrintTemplateArguments(const std::vector<sptr<TypeSpecifier>>& args);
    void PrintStringLiteral(std::string_view value);

    static Precedence GetPrecedence(const sptr<Expression>& expr);
    static Precedence GetBinaryPrecedence(TokenType operation);
    static bool IsMultiLine(const sptr<Definition>& def);
    static bool StartsWithSign(const sptr<Expression>& expr);
public:
    CodePrinter(std::ostream& stream, int tabWidth = 4, std::optional<std::string_view> printedFile = {}, int startingIndent = 0)
        : stream(stream)
        , tabWidth(tabWidth)
        , indent(startingIndent)
        , printedFile(printedFile)
    {
    }

    static void Print(const sptr<ASTNode>& node) {
        std::stringstream stream;
        fraze::CodePrinter printer(stream, 4);
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

    virtual void Visit(const sptr<ArrayCountExpression>& node) override;
    virtual void Visit(const sptr<AsExpression>& node) override;
    virtual void Visit(const sptr<AssignExpression>& node) override;
    virtual void Visit(const sptr<AwaitExpression>& node) override;
    virtual void Visit(const sptr<BinaryExpression>& node) override;
    virtual void Visit(const sptr<BooleanLiteralExpression>& node) override;
    virtual void Visit(const sptr<CastExpression>& node) override;
    virtual void Visit(const sptr<CallExpression>& node) override;
    virtual void Visit(const sptr<CheckSiteExpression>& node) override;
    virtual void Visit(const sptr<ConvertExpression>& node) override;
    virtual void Visit(const sptr<DefaultValueExpression>& node) override;
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
