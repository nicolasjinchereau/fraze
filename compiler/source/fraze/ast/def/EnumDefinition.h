/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/type/Type.h>
#include <fraze/common/Object.h>
#include <fraze/common/Scope.h>

namespace fraze {

class EnumDefinition : public Definition
{
public:
    Type* type{};
    // members in Definition::children as instances of EnumMemberDefinition

    EnumDefinition(
        const SourceLocation& loc
        , Scope* enclosingScope
        , const shared_string& name)
        : Definition(loc, enclosingScope, name)
    {
        scope = spnew<Scope>();
        scope->parent = enclosingScope;
        scope->owner = this;
        type = Type::Get(this);
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override;

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<EnumDefinition> ToEnumDefinition() override
    {
        return self();
    }
};

} // fraze
