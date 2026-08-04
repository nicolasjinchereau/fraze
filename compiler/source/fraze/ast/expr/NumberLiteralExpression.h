/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>

namespace fraze {

class NumberLiteralExpression : public Expression
{
public:
    double value{};

    NumberLiteralExpression(const SourceLocation& loc, Scope* scope, double value = 0)
        : Expression(loc, scope), value(value){}

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<NumberLiteralExpression>(loc, scopes.GetCurrent(), value);

        copy->pushAsRef = pushAsRef;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<NumberLiteralExpression> ToNumberLiteralExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return Type::Get("num");
    }
};

} // fraze
