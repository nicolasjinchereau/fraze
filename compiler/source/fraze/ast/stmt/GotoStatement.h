/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/ASTNode.h>
#include <fraze/ast/stmt/Statement.h>
#include <fraze/ast/expr/Expression.h>

namespace fraze {

class GotoStatement : public Statement
{
public:
    sptr<Expression> expression;

    GotoStatement(const SourceLocation& loc, Scope* enclosingScope)
        : Statement(loc, enclosingScope)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<GotoStatement>(loc, scopes.GetCurrent());

        copy->expression = expression ? expression->Clone(scopes, nullptr)->ToExpression() : decltype(expression){};

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<GotoStatement> ToGotoStatement() override
    {
        return self();
    }
};

} // fraze
