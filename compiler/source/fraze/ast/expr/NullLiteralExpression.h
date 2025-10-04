/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>

namespace fraze {

class NullLiteralExpression : public Expression
{
public:
    NullLiteralExpression(const SourceLocation& loc, Scope* scope)
        : Expression(loc, scope)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<NullLiteralExpression>(loc, scopes.GetCurrent());

        copy->isContext = isContext;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<NullLiteralExpression> ToNullLiteralExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return Type::Get("null");
    }
};

} // fraze
