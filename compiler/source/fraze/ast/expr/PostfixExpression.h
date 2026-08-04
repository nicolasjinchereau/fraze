/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>
#include <fraze/compiler/Lexer.h>

namespace fraze {

class PostfixExpression : public Expression
{
public:
    sptr<Expression> arg;
    TokenType operation{};

    PostfixExpression(const SourceLocation& loc, Scope* scope, const sptr<Expression>& arg, TokenType operation)
        : Expression(loc, scope), arg(arg), operation(operation)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<PostfixExpression>(loc, scopes.GetCurrent(),
                arg ? arg->Clone(scopes, nullptr)->ToExpression() : decltype(arg){},
                operation);

        copy->pushAsRef = pushAsRef;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<PostfixExpression> ToPostfixExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return arg->EvaluateType();
    }
};

} // fraze
