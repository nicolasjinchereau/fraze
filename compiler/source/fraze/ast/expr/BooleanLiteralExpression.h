/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>

namespace fraze {

class BooleanLiteralExpression : public Expression
{
public:
    bool value;

    BooleanLiteralExpression(const SourceLocation& loc, Scope* scope, bool value = 0)
        : Expression(loc, scope), value(value){}

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<BooleanLiteralExpression>(loc, scopes.GetCurrent(), value);

        copy->pushAsRef = pushAsRef;

        return copy;
    }
    
    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<BooleanLiteralExpression> ToBooleanLiteralExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return Type::Get("bool");
    }
};

} // fraze
