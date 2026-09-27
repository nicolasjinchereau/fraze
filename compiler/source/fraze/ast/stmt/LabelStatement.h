/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/stmt/Statement.h>
#include <fraze/common/SharedString.h>

namespace fraze {

class LabelStatement : public Statement
{
public:
    shared_string name;

    LabelStatement(const SourceLocation& loc, Scope* enclosingScope, const shared_string& name)
        : Statement(loc, enclosingScope), name(name)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<LabelStatement>(loc, scopes.GetCurrent(), name);

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<LabelStatement> ToLabelStatement() override
    {
        return self();
    }
};

} // fraze
