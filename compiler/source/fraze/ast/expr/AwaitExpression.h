/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/ASTNode.h>
#include <fraze/ast/stmt/Statement.h>
#include <fraze/ast/expr/Expression.h>
#include <fraze/ast/expr/NullLiteralExpression.h>
#include <fraze/ast/expr/IdentifierExpression.h>

namespace fraze {

class AwaitExpression : public Expression
{
public:
    sptr<Expression> expression;
    sptr<IdentifierExpression> context; // 'this' refering to the task enclosure

    AwaitExpression(const SourceLocation& loc, Scope* enclosingScope, const sptr<Expression>& expression)
        : Expression(loc, enclosingScope), expression(expression)
    {
        context = spnew<IdentifierExpression>(loc, enclosingScope, shared_string("this"));
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<AwaitExpression>(loc, scope, expression->Clone(scopes, nullptr)->ToAwaitExpression());

        copy->pushAsRef = pushAsRef;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<AwaitExpression> ToAwaitExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        Type* taskType = expression->EvaluateType();
        assert(taskType->GetName().starts_with("Task"));

        auto& templateArgs = taskType->GetTemplateArgs();
        assert(templateArgs.size() == 1);

        return templateArgs.back();
    }
};

} // fraze
