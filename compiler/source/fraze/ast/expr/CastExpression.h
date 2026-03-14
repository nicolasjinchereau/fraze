/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>
#include <fraze/compiler/Lexer.h>

namespace fraze {

class CastExpression : public Expression
{
public:
    sptr<TypeSpecifier> resultTypeSpec;
    sptr<Expression> value;

    CastExpression(
        const SourceLocation& loc
        , Scope* scope
        , const sptr<TypeSpecifier>& resultTypeSpec
        , const sptr<Expression>& value)
        : Expression(loc, scope), resultTypeSpec(resultTypeSpec), value(value)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<CastExpression>(
                loc, scopes.GetCurrent(),
                resultTypeSpec->Clone(scopes, nullptr)->ToTypeSpecifier(),
                value->Clone(scopes, nullptr)->ToExpression());
        
        copy->isContext = isContext;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<CastExpression> ToCastExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return resultTypeSpec->type;
    }
};

} // fraze
