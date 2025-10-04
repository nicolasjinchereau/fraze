/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/ASTRoot.h>
#include <fraze/ast/def/FunctionDefinition.h>
#include <fraze/ast/def/SectionDefinition.h>
#include <fraze/ast/def/VariableDefinition.h>
#include <fraze/ast/expr/Expression.h>
#include <fraze/ast/expr/IdentifierExpression.h>
#include <fraze/ast/stmt/VariableDefinitionStatement.h>
#include <fraze/common/SharedString.h>

namespace fraze {

class CachedExpression : public Expression
{
public:
    sptr<VariableDefinitionStatement> cache;
    sptr<IdentifierExpression> value;

    CachedExpression(const sptr<Expression>& expr, const shared_string& name, bool isContext = false)
        : Expression(expr->loc, expr->scope)
    {
        auto exprType =  expr->EvaluateType();
        auto typeSpec = spnew<TypeSpecifier>(loc, exprType);

        cache = spnew<VariableDefinitionStatement>(loc, scope);
        cache->variableDefinition = spnew<VariableDefinition>(loc, scope, typeSpec, name);
        cache->variableDefinition->initializer = expr;
        scope->AddDefinition(cache->variableDefinition);

        value = spnew<IdentifierExpression>(loc, scope, name);
        value->isContext = isContext;
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<CachedExpression>(
                cache->variableDefinition->initializer->Clone(scopes, nullptr)->ToExpression(),
                value->value,
                value->isContext);

        copy->isContext = isContext;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<CachedExpression> ToCachedExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return value->EvaluateType();
    }
};

} // fraze
