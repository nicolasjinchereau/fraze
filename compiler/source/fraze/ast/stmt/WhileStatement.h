/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/IdentifierExpression.h>
#include <fraze/ast/stmt/Statement.h>
#include <fraze/common/Scope.h>

namespace fraze {

class WhileStatement : public Statement
{
public:
    sptr<Scope> scope;
    sptr<Expression> condition;
    sptr<Statement> body;

    WhileStatement(const SourceLocation& loc, Scope* enclosingScope)
        : Statement(loc, enclosingScope)
    {
        scope = spnew<Scope>();
        scope->parent = enclosingScope;
        scope->owner = enclosingScope->owner;
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<WhileStatement>(loc, scopes.GetCurrent());
        
        scopes.Push(copy->scope.get());

        copy->condition = condition ? condition->Clone(scopes, nullptr)->ToExpression() : decltype(condition){};
        copy->body = body ? body->Clone(scopes, nullptr)->ToStatement() : decltype(body){};

        scopes.Pop();

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<WhileStatement> ToWhileStatement() override
    {
        return self();
    }
};

} // fraze
