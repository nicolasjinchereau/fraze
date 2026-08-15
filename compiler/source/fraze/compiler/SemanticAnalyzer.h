/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
#include <fraze/ast/ASTVisitor.h>
#include <fraze/ast/AST.h>
#include <fraze/common/Scope.h>

namespace fraze {

class Compiler;
class Type;

class SemanticAnalyzer : public ASTVisitor
{
    sptr<ASTRoot> astRoot;
    int nextUniqueId = 0;
public:

    SemanticAnalyzer();

    /*****************************
    *            ROOT            *
    *****************************/

    virtual void Visit(const sptr<ASTRoot>& node) override;

    /*****************************
    *         DEFINITIONS        *
    *****************************/

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

    /*****************************
    *         EXPRESSIONS        *
    *****************************/

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

    /*****************************
    *            TYPES           *
    *****************************/

    // resolves Type for TypeSpecifier
    virtual void Visit(const sptr<TypeSpecifier>& node) override;

    /****************************
    *         STATEMENTS        *
    ****************************/

    virtual void Visit(const sptr<AssertStatement>& node);
    virtual void Visit(const sptr<EmptyStatement>& node);
    virtual void Visit(const sptr<GotoStatement>& node);
    virtual void Visit(const sptr<ReturnStatement>& node);

private:
    Type* EvaluateTypeChecked(const sptr<Expression>& expr);
    bool IsFunctionAssignable(const sptr<FunctionDefinition>& leftFunc, const sptr<FunctionDefinition>& rightFunc, bool skipImplicitThisParam = false);
    bool IsAssignable(Type* leftType, Type* rightType, TokenType operation = TokenType::Assign, bool* needsConversion = nullptr);
    void ProcessAssignment(const SourceLocation& leftLoc, Type* left, sptr<Expression>& right, TokenType operation = TokenType::Assign);
    void ProcessBinaryOperation(const sptr<BinaryExpression>& node);
    void ProcessCondition(sptr<Expression>& expr);
    std::pair<sptr<FunctionDefinition>, bool> GetBinaryOperatorOverload(Type* structType, TokenType operation, const sptr<Expression>& left, const sptr<Expression>& right);
    sptr<FunctionDefinition> CreateEqualityOperator(Type* structType, TokenType operation);
    sptr<FunctionDefinition> GetUnaryOperatorOverload(Type* structType, TokenType operation, const sptr<Expression>& value);
    std::optional<sptr<ASTNode>> CreateAsInstanceCall(const sptr<AsExpression>& node);
    std::optional<sptr<ASTNode>> CreateStringFromTypeCall(const sptr<AsExpression>& node);

    sptr<FunctionDefinition> FindDefinition(Scope* scope, const std::string& name, const std::vector<sptr<Expression>>& arguments);
    sptr<Definition> SearchUpward(Scope* scope, const std::string& name, const std::vector<sptr<Expression>>& arguments, Scope* toScope = nullptr);
    
    sptr<Definition> FindIdentifierTarget(const sptr<IdentifierExpression>& node);

    sptr<FunctionDefinition> GetMatchingFunction(const sptr<ClassDefinition>& classDef, const sptr<InterfaceDefinition>& interfaceDef, const sptr<FunctionDefinition>& interfaceFunc);
    void FindCallTargets(std::string_view name, Scope* fromScope, Scope* toScope, std::vector<sptr<Definition>>& callTargets);
    std::vector<sptr<Definition>> FindAllCallTargets(const sptr<IdentifierExpression>& node);
    sptr<Definition> SelectCallTarget(std::string_view name,const SourceLocation& loc, const Scope* scope, const std::vector<sptr<Definition>>& callTargets, const sptr<Expression>& context, const std::vector<sptr<Expression>>& arguments, bool suppressErrors = false);
    sptr<FunctionDefinition> GetCallTargetFunction(const sptr<Definition>& target);
    sptr<FunctionDefinition> InstantiateTemplateFunction(const sptr<IdentifierExpression>& node, const sptr<FunctionDefinition>& templateFunc);
    void ApplyTemplateArguments(const sptr<IdentifierExpression>& node, const std::vector<sptr<Expression>>& arguments, std::vector<sptr<Definition>>& callTargets);
    sptr<ClassDefinition> GetCoroutineOriginalClass(const sptr<ClassDefinition>& coroutineState);
    void VisitCallTarget(const sptr<IdentifierExpression>& node, std::vector<sptr<Expression>>& arguments);
    sptr<Expression> WrapWithNullCheck(const sptr<Expression>& value);

    size_t GetVarSize(Type* type);

    void ParseCodeString(Scope* enclosingScope, const std::string& code);
};

} // fraze
