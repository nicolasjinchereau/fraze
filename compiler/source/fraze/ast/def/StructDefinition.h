/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/def/VariableDefinition.h>
#include <fraze/ast/def/PropertyDefinition.h>
#include <fraze/ast/def/FunctionDefinition.h>
#include <fraze/ast/def/InterfaceDefinition.h>
#include <fraze/ast/def/TemplateParameterDefinition.h>
#include <fraze/ast/def/TemplateDefinition.h>
#include <fraze/ast/type/Type.h>
#include <fraze/common/Object.h>
#include <fraze/common/Scope.h>

namespace fraze {

class StructDefinition : public TemplateDefinition
{
public:
    Type* type{};
    size_t size{};

    StructDefinition(
        const SourceLocation& loc
        , Scope* enclosingScope
        , const shared_string& name
        , const std::vector<sptr<TypeSpecifier>>& templateArgs = {})
        : TemplateDefinition(loc, enclosingScope, name, templateArgs)
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

    virtual sptr<StructDefinition> ToStructDefinition() override
    {
        return self();
    }
};

} // fraze
