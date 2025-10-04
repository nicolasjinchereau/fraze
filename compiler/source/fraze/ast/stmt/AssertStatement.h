/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/IdentifierExpression.h>
#include <fraze/ast/stmt/Statement.h>
#include <fraze/common/Scope.h>

namespace fraze {

class AssertStatement : public Statement
{
public:
    sptr<Expression> condition; // must evaluate to a boolean
    sptr<Expression> message; // must evaluate to a string 

    AssertStatement(const SourceLocation& loc, Scope* enclosingScope, const sptr<Expression>& condition, const sptr<Expression>& message)
        : Statement(loc, enclosingScope), condition(condition), message(message)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<AssertStatement>(
                loc, scopes.GetCurrent(),
                condition->Clone(scopes, nullptr)->ToExpression(),
                message ? message->Clone(scopes, nullptr)->ToExpression() : decltype(message){});

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<AssertStatement> ToAssertStatement() override
    {
        return self();
    }
};

} // fraze
