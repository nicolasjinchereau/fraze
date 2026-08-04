/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>

namespace fraze {

class IntegerLiteralExpression : public Expression
{
public:
    int64_t value{};

    IntegerLiteralExpression(const SourceLocation& loc, Scope* scope, int64_t value = 0)
        : Expression(loc, scope), value(value){}

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<IntegerLiteralExpression>(loc, scopes.GetCurrent(), value);

        copy->pushAsRef = pushAsRef;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<IntegerLiteralExpression> ToIntegerLiteralExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return Type::Get("int");
    }
};

} // fraze
