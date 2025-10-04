/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/stmt/Statement.h>
#include <fraze/ast/expr/IdentifierExpression.h>

namespace fraze {

class ExposeStatement : public Statement
{
public:
    sptr<TypeSpecifier> section;

    ExposeStatement(const SourceLocation& loc, Scope* enclosingScope)
        : Statement(loc, enclosingScope)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<ExposeStatement>(loc, scopes.GetCurrent());

        copy->section = section ? section->Clone(scopes, nullptr)->ToTypeSpecifier() : decltype(section){};

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<ExposeStatement> ToExposeStatement() override
    {
        return self();
    }
};

} // fraze
