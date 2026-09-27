/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/ASTNode.h>
#include <fraze/ast/stmt/Statement.h>
#include <fraze/ast/expr/Expression.h>

namespace fraze {

class ReturnStatement : public Statement
{
public:
    sptr<Expression> expression;

    // Indicates this return was written in a coroutine and will be lowered to a block that
    // stores its expression in the coroutine's task, marks it done, and calls ResumeAwaiter.
    bool isCoroutineCompletion = false;

    ReturnStatement(const SourceLocation& loc, Scope* enclosingScope, const sptr<Expression>& expression = {})
        : Statement(loc, enclosingScope), expression(expression)
    {
        auto func = enclosingScope->owner->ToFunctionDefinition();
        isCoroutineCompletion = func && func->isCoroutine;
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<ReturnStatement>(
            loc, scopes.GetCurrent(),
            expression ? expression->Clone(scopes, nullptr)->ToExpression() : decltype(expression){});

        copy->isCoroutineCompletion = isCoroutineCompletion;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<ReturnStatement> ToReturnStatement() override
    {
        return self();
    }
};

} // fraze
