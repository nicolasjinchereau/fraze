/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <string>
#include <vector>
#include <fraze/ast/expr/Expression.h>

namespace fraze {

class IndexExpression : public Expression
{
public:
    sptr<Expression> target;
    sptr<Expression> arg;

    IndexExpression(const SourceLocation& loc, Scope* scope, const sptr<Expression>& target)
        : Expression(loc, scope), target(target)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<IndexExpression>(loc, scopes.GetCurrent(), target->Clone(scopes, nullptr)->ToExpression());

        copy->isContext = isContext;
        copy->arg = arg ? arg->Clone(scopes, nullptr)->ToExpression() : decltype(arg){};

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<IndexExpression> ToIndexExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        Type* targetArrayType = target->EvaluateType();
        return targetArrayType->GetElementType();
    }
};

} // fraze
