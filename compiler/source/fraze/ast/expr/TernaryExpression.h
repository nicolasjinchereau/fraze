/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>
#include <fraze/compiler/Lexer.h>

namespace fraze {

class TernaryExpression : public Expression
{
public:
    sptr<Expression> condition;
    sptr<Expression> trueValue;
    sptr<Expression> falseValue;

    TernaryExpression(
        const SourceLocation& loc,
        Scope* scope,
        const sptr<Expression>& condition,
        const sptr<Expression>& trueValue,
        const sptr<Expression>& falseValue)
        : Expression(loc, scope)
        , condition(condition)
        , trueValue(trueValue)
        , falseValue(falseValue)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<TernaryExpression>(
                loc,
                scopes.GetCurrent(),
                condition->Clone(scopes, nullptr)->ToExpression(),
                trueValue->Clone(scopes, nullptr)->ToExpression(),
                falseValue->Clone(scopes, nullptr)->ToExpression());

        copy->pushAsRef = pushAsRef;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<TernaryExpression> ToTernaryExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return trueValue->EvaluateType();
    }
};

} // fraze
