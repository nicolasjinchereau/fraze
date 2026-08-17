/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>
#include <string>

namespace fraze {

class TypeLiteralExpression : public Expression
{
public:
    shared_string value;
    size_t runtimeValue{}; // set up by code generator

    TypeLiteralExpression(const SourceLocation& loc, Scope* scope, const shared_string& value = {})
        : Expression(loc, scope), value(value){}

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<TypeLiteralExpression>(loc, scopes.GetCurrent(), value);

        copy->pushAsRef = pushAsRef;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<TypeLiteralExpression> ToTypeLiteralExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return Type::Get("int");
    }
};

} // fraze
