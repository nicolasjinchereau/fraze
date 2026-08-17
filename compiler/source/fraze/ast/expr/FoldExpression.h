/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>
#include <fraze/ast/stmt/BlockStatement.h>
#include <fraze/ast/stmt/ExpressionStatement.h>

namespace fraze {

class FoldExpression : public Expression
{
public:
    // last statement must be an ExpressionStatement
    sptr<BlockStatement> body;

    FoldExpression(const SourceLocation& loc, Scope* enclosingScope, const sptr<BlockStatement>& body)
        : Expression(loc, enclosingScope), body(body)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<FoldExpression>(
                loc,
                scopes.GetCurrent(),
                body->Clone(scopes, nullptr)->ToBlockStatement());

        copy->pushAsRef = pushAsRef;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<FoldExpression> ToFoldExpression() override
    {
        return self();
    }

    sptr<Expression> GetFinalExpression()
    {
        assert(body);
        assert(!body->statements.empty());
        auto finalStatement = body->statements.back()->ToExpressionStatement();
        return finalStatement->expression;
    }

    virtual Type* EvaluateType() override
    {
        return GetFinalExpression()->EvaluateType();
    }
};

} // fraze
