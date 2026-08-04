/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>
#include <fraze/compiler/Lexer.h>

namespace fraze {

class AsExpression : public Expression
{
public:
    sptr<Expression> value;
    sptr<TypeSpecifier> typeSpec;

    AsExpression(const SourceLocation& loc, Scope* scope, const sptr<Expression>& value, const sptr<TypeSpecifier>& typeSpec)
        : Expression(loc, scope), value(value), typeSpec(typeSpec)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<AsExpression>(
                loc,
                scopes.GetCurrent(),
                value->Clone(scopes, nullptr)->ToExpression(),
                typeSpec->Clone(scopes, nullptr)->ToTypeSpecifier());

        copy->pushAsRef = pushAsRef;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<AsExpression> ToAsExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return typeSpec->GetType();
    }
};

} // fraze
