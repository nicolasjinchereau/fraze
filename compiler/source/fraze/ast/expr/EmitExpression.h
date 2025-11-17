/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>

namespace fraze {

struct Emission
{
    SourceLocation loc;
    Operation op;
};

class EmitExpression : public Expression
{
public:
    sptr<TypeSpecifier> expectedType;
    sptr<Expression> context;
    std::vector<Emission> emissions;

    EmitExpression(
        const SourceLocation& loc,
        Scope* scope,
        const sptr<TypeSpecifier>& expectedType,
        const sptr<Expression>& context,
        const Emission& emission)
        : Expression(loc, scope)
        , expectedType(expectedType)
        , context(context)
        , emissions({ emission })
    {
    }

    EmitExpression(
        const SourceLocation& loc,
        Scope* scope,
        const Emission& emission)
        : Expression(loc, scope)
        , expectedType(spnew<TypeSpecifier>(loc, Type::Get("void")))
        , context(nullptr)
        , emissions({ emission })
    {
    }

    EmitExpression(
        const SourceLocation& loc,
        Scope* scope,
        const sptr<TypeSpecifier>& expectedType,
        const sptr<Expression>& context,
        const std::vector<Emission>& emissions)
        : Expression(loc, scope)
        , expectedType(expectedType)
        , context(context)
        , emissions(emissions)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<EmitExpression>(
                loc,
                scopes.GetCurrent(),
                expectedType->Clone(scopes, nullptr)->ToTypeSpecifier(),
                context ? context->Clone(scopes, nullptr)->ToExpression() : decltype(context){},
                emissions);

        copy->isContext = isContext;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<EmitExpression> ToEmitExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return expectedType->type;
    }
};

} // fraze
