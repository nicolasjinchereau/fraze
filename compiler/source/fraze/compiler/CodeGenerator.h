/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cassert>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <fraze/ast/AST.h>
#include <fraze/ast/ASTVisitor.h>
#include <fraze/program/Program.h>

namespace fraze {

class Compiler;

class CodeGenerator : public ASTVisitor
{
    std::unordered_map<const Type*, sptr<TypeInfo>> typeInfo;
public:
    sptr<Program> program;

    size_t Emit(nullptr_t data);
    size_t Emit(Boolean data);
    size_t Emit(Integer data);
    size_t Emit(Number data);
    size_t Emit(String* data);

    void Emit(const SourceLocation& loc, const Operation& op) {
        program->locations.push_back(loc);
        program->code.push_back(op);
    }

    void Emit(const SourceLocation& loc, OpCode op) {
        program->locations.push_back(loc);
        program->code.push_back(Operation(op, 0ull, 0ull));
    }

    void Emit(const SourceLocation& loc, OpCode op, auto arg1) {
        program->locations.push_back(loc);
        program->code.push_back(Operation(op, arg1, 0ull));
    }

    void Emit(const SourceLocation& loc, OpCode op, auto arg1, auto arg2) {
        program->locations.push_back(loc);
        program->code.push_back(Operation(op, arg1, arg2));
    }

    void PopExpression(const sptr<Expression>& node, const sptr<Expression>& source);
    void EmitConversion(sptr<Expression>& value, const sptr<TypeSpecifier>& resultType);

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

    /****************************
    *         SPECIFIERS        *
    ****************************/

    virtual void Visit(const sptr<TypeSpecifier>& node) override;

    /****************************
    *         STATEMENTS        *
    ****************************/

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
