/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>
#include <fraze/compiler/Lexer.h>

namespace fraze {

class NewExpression : public Expression
{
public:
    sptr<TypeSpecifier> typeSpec;
    std::vector<sptr<Expression>> arguments;
    sptr<Expression> argumentExpression;
    std::optional<bool> hasConstructor;

    NewExpression(const SourceLocation& loc, Scope* scope)
        : Expression(loc, scope)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<NewExpression>(loc, scopes.GetCurrent());

        copy->isContext = isContext;
        copy->typeSpec = typeSpec ? typeSpec->Clone(scopes, nullptr)->ToTypeSpecifier() : decltype(typeSpec){};

        for(auto& arg : arguments)
        {
            auto argCopy = arg->Clone(scopes, nullptr)->ToExpression();
            copy->arguments.push_back(argCopy);
        }

        copy->argumentExpression = argumentExpression ? argumentExpression->Clone(scopes, nullptr)->ToExpression() : decltype(argumentExpression){};

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<NewExpression> ToNewExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return typeSpec->GetType();
    }
};

} // fraze
