/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>
#include <print>

namespace fraze {

class DefaultValueExpression : public Expression
{
public:
    sptr<TypeSpecifier> typeSpec;
    sptr<AssignExpression> coroutineFieldInit;

    DefaultValueExpression(
        const SourceLocation& loc
        , Scope* scope
        , const sptr<TypeSpecifier>& typeSpec)
        : Expression(loc, scope), typeSpec(typeSpec)
    {
    }

    DefaultValueExpression(
        const SourceLocation& loc
        , Scope* scope
        , const sptr<AssignExpression>& coroutineFieldInit)
        : Expression(loc, scope)
        , coroutineFieldInit(coroutineFieldInit)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<DefaultValueExpression>(loc, scopes.GetCurrent(),
                typeSpec->Clone(scopes, nullptr)->ToTypeSpecifier());

        copy->pushAsRef = pushAsRef;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<DefaultValueExpression> ToDefaultValueExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return typeSpec->type;
    }
};

} // fraze
