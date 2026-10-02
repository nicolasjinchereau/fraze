/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <vector>
#include <string>
#include <memory>
#include <fraze/ast/def/ClassDefinition.h>
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/def/FunctionDefinition.h>
#include <fraze/ast/def/VariableDefinition.h>
#include <fraze/common/Scope.h>

namespace fraze
{

class SectionDefinition : public Definition
{
public:
    Type* type{};

    SectionDefinition(const SourceLocation& loc, Scope* enclosingScope, const shared_string& name)
        : Definition(loc, enclosingScope, name)
    {
        scope = spnew<Scope>();
        scope->parent = enclosingScope;
        scope->owner = this;
        type = Type::Get(this);
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<SectionDefinition>(loc, scopes.GetCurrent(), name);

        scopes.GetCurrent()->AddDefinition(copy);

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<SectionDefinition> ToSectionDefinition() override
    {
        return self();
    }
};

} // fraze
