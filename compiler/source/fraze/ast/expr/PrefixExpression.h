/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>
#include <fraze/compiler/Lexer.h>

namespace fraze {

class PrefixExpression : public Expression
{
public:
    TokenType operation;
    sptr<Expression> arg;

    PrefixExpression(const SourceLocation& loc, Scope* scope, TokenType op, const sptr<Expression>& arg)
        : Expression(loc, scope), operation(op), arg(arg)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<PrefixExpression>(loc, scopes.GetCurrent(),
                operation,
                arg ? arg->Clone(scopes, nullptr)->ToExpression() : decltype(arg){});

        copy->isContext = isContext;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<PrefixExpression> ToPrefixExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        if (operation == TokenType::LogicalNot)
        {
            return Type::Get("bool");
        }
        else
        {
            return arg->EvaluateType();
        }
    }
};

} // fraze
