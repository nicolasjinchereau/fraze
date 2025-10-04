/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/IdentifierExpression.h>
#include <fraze/ast/stmt/Statement.h>
#include <fraze/ast/stmt/ExpressionStatement.h>
#include <fraze/common/Scope.h>

namespace fraze {

class ForStatement : public Statement
{
public:
    sptr<Scope> scope;
    sptr<Statement> init;
    sptr<Expression> condition;
    sptr<ExpressionStatement> iterate;
    sptr<Statement> body;

    ForStatement(const SourceLocation& loc, Scope* enclosingScope)
        : Statement(loc, enclosingScope)
    {
        scope = spnew<Scope>();
        scope->parent = enclosingScope;
        scope->owner = enclosingScope->owner;
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<ForStatement>(loc, scopes.GetCurrent());
        
        scopes.Push(copy->scope.get());

        copy->init = init ? init->Clone(scopes, nullptr)->ToStatement() : decltype(init){};
        copy->condition = condition ? condition->Clone(scopes, nullptr)->ToExpression() : decltype(condition){};
        copy->iterate = iterate ? iterate->Clone(scopes, nullptr)->ToExpressionStatement() : decltype(iterate){};
        copy->body = body ? body->Clone(scopes, nullptr)->ToStatement() : decltype(body){};

        scopes.Pop();

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<ForStatement> ToForStatement() override
    {
        return self();
    }
};

} // fraze
