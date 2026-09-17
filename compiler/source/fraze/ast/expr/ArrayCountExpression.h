/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>

namespace fraze {

// the element count of an array, read directly from the array object's header
class ArrayCountExpression : public Expression
{
public:
    sptr<Expression> array;

    ArrayCountExpression(const SourceLocation& loc, Scope* scope, const sptr<Expression>& array)
        : Expression(loc, scope), array(array){}

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<ArrayCountExpression>(
            loc, scopes.GetCurrent(),
            array ? array->Clone(scopes, nullptr)->ToExpression() : decltype(array){});

        copy->pushAsRef = pushAsRef;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<ArrayCountExpression> ToArrayCountExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return Type::Get("int");
    }
};

} // fraze
