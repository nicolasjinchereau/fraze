/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>
#include <fraze/compiler/Lexer.h>

namespace fraze {

class AssignExpression : public Expression
{
public:
    TokenType operation;
    sptr<Expression> left;
    sptr<Expression> right;
    VariableDefinition* fieldToInitialize{};

    AssignExpression(const SourceLocation& loc, Scope* scope, TokenType op, const sptr<Expression>& left, const sptr<Expression>& right)
        : Expression(loc, scope), operation(op), left(left), right(right)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<AssignExpression>(
                loc, scopes.GetCurrent(), operation,
                left->Clone(scopes, nullptr)->ToExpression(),
                right->Clone(scopes, nullptr)->ToExpression());

        copy->pushAsRef = pushAsRef;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<AssignExpression> ToAssignExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return left->EvaluateType();
    }
};

} // fraze
