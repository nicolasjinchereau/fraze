/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>
#include <fraze/compiler/Lexer.h>

namespace fraze {

class ConvertExpression : public Expression
{
public:
    sptr<TypeSpecifier> resultTypeSpec;
    sptr<Expression> value;

    ConvertExpression(
        const SourceLocation& loc
        , Scope* scope
        , const sptr<TypeSpecifier>& resultTypeSpec
        , const sptr<Expression>& value)
        : Expression(loc, scope), resultTypeSpec(resultTypeSpec), value(value)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<ConvertExpression>(
                loc, scopes.GetCurrent(),
                resultTypeSpec->Clone(scopes, nullptr)->ToTypeSpecifier(),
                value->Clone(scopes, nullptr)->ToExpression());
        
        copy->pushAsRef = pushAsRef;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<ConvertExpression> ToConvertExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return resultTypeSpec->type;
    }
};

} // fraze
