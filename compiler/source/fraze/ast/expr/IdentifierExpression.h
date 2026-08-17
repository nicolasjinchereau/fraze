/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/expr/Expression.h>
#include <fraze/ast/type/TypeSpecifier.h>
#include <fraze/compiler/Lexer.h>
#include <vector>

namespace fraze {

class Scope;

class IdentifierExpression : public Expression
{
public:
    shared_string value;
    sptr<Expression> context;
    std::vector<sptr<TypeSpecifier>> templateArgs;
    Definition* targetDef{};

    IdentifierExpression(const SourceLocation& loc, Scope* scope, const shared_string& value)
        : Expression(loc, scope), value(value)
    {
    }

    IdentifierExpression(const SourceLocation& loc, Scope* scope, const sptr<Expression>& context, const shared_string& value)
        : Expression(loc, scope), context(context), value(value)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<IdentifierExpression>(
                loc, scopes.GetCurrent(),
                context ? context->Clone(scopes, nullptr)->ToExpression() : decltype(context){},
                value);

        copy->pushAsRef = pushAsRef;
        copy->templateArgs = TypeSpecifier::CloneTypeSpecs(scopes, templateArgs);

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<IdentifierExpression> ToIdentifierExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override;

    Definition* GetTargetDefinition() const;
};

} // fraze
