/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/IdentifierExpression.h>
#include <fraze/ast/stmt/Statement.h>
#include <fraze/common/Scope.h>

namespace fraze {

class IfStatement : public Statement
{
public:
    sptr<Scope> scope;
    sptr<Expression> condition;
    sptr<Statement> trueBranch;
    sptr<Statement> falseBranch;

    IfStatement(const SourceLocation& loc, Scope* enclosingScope)
        : Statement(loc, enclosingScope)
    {
        scope = spnew<Scope>();
        scope->parent = enclosingScope;
        scope->owner = enclosingScope->owner;
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<IfStatement>(loc, scopes.GetCurrent());

        scopes.Push(copy->scope.get());

        copy->condition = condition ? condition->Clone(scopes, nullptr)->ToExpression() : decltype(condition){};
        copy->trueBranch = trueBranch ? trueBranch->Clone(scopes, nullptr)->ToStatement() : decltype(trueBranch){};
        copy->falseBranch = falseBranch ? falseBranch->Clone(scopes, nullptr)->ToStatement() : decltype(falseBranch){};

        scopes.Pop();

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<IfStatement> ToIfStatement() override
    {
        return self();
    }
};

} // fraze
