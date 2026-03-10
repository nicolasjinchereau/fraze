/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>
#include <fraze/compiler/Lexer.h>

namespace fraze {

class TypeOfExpression : public Expression
{
public:
    sptr<TypeSpecifier> typeSpec;

    TypeOfExpression(const SourceLocation& loc, Scope* scope, const sptr<TypeSpecifier>& typeSpec)
        : Expression(loc, scope), typeSpec(typeSpec)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<SizeOfExpression>(
            loc, scopes.GetCurrent(),
            typeSpec ? typeSpec->Clone(scopes, nullptr)->ToTypeSpecifier() : decltype(typeSpec){});

        copy->isContext = isContext;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<TypeOfExpression> ToTypeOfExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return Type::Get("Type");
    }
};

} // fraze
