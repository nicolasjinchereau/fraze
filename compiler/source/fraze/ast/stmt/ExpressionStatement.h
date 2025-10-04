/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <memory>
#include <fraze/ast/expr/Expression.h>
#include <fraze/ast/stmt/Statement.h>

namespace fraze {

class ExpressionStatement : public Statement
{
public:
    sptr<Expression> expression;

    ExpressionStatement(const sptr<Expression>& expr, Scope* enclosingScope)
        : Statement(expr->loc, enclosingScope), expression(expr)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<ExpressionStatement>(expression->Clone(scopes, nullptr)->ToExpression(), scopes.GetCurrent());

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<ExpressionStatement> ToExpressionStatement() override
    {
        return self();
    }
};

} // fraze
