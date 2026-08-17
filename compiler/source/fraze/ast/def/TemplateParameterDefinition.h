/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <string>
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/type/Type.h>

namespace fraze {

class ClassDefinition;

class TemplateParameterDefinition : public Definition
{
public:
    sptr<TypeSpecifier> typeSpec;
    
    TemplateParameterDefinition(
        const SourceLocation& loc,
        Scope* enclosingScope,
        const shared_string& name,
        const sptr<TypeSpecifier>& typeSpec)
        : Definition(loc, enclosingScope, name), typeSpec(typeSpec)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<TemplateParameterDefinition>(loc, scopes.GetCurrent(), name, typeSpec->Clone(scopes, nullptr)->ToTypeSpecifier());
        
        scopes.GetCurrent()->AddDefinition(copy);
        
        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<TemplateParameterDefinition> ToTemplateParameterDefinition() override
    {
        return self();
    }
};

} // fraze
