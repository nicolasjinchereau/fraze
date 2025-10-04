/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>
#include <string>

namespace fraze {

class StringLiteralExpression : public Expression
{
public:
    shared_string value;

    StringLiteralExpression(const SourceLocation& loc, Scope* scope, const shared_string& value = {})
        : Expression(loc, scope), value(value){}

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<StringLiteralExpression>(loc, scopes.GetCurrent(), value);

        copy->isContext = isContext;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<StringLiteralExpression> ToStringLiteralExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return Type::Get("string");
    }
};

} // fraze
