/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>

namespace fraze {

// evaluates to the id of a runtime check site (message + this node's location);
// CodeGenerator assigns the id and records the site in Program::checkSites
class CheckSiteExpression : public Expression
{
public:
    shared_string message; // empty for asserts, which pass their message at runtime

    CheckSiteExpression(const SourceLocation& loc, Scope* scope, shared_string message = shared_string())
        : Expression(loc, scope), message(message){}

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<CheckSiteExpression>(loc, scopes.GetCurrent(), message);

        copy->pushAsRef = pushAsRef;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<CheckSiteExpression> ToCheckSiteExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        return Type::Get("int");
    }
};

} // fraze
