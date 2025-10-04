/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <string>
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/type/TypeSpecifier.h>

namespace fraze {

class ParameterDefinition : public Definition
{
public:
    sptr<TypeSpecifier> typeSpec;
    size_t offset = 0;
    size_t size = 0;

    ParameterDefinition(const SourceLocation& loc, Scope* enclosingScope, const sptr<TypeSpecifier> typeSpec, const shared_string& name)
        : Definition(loc, enclosingScope, name), typeSpec(typeSpec)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<ParameterDefinition>(loc, scopes.GetCurrent(), typeSpec->Clone(scopes, nullptr)->ToTypeSpecifier(), name);

        scopes.GetCurrent()->AddDefinition(copy);

        copy->offset = offset;
        copy->size = size;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override {
        visitor.Visit(self());
    }

    virtual sptr<ParameterDefinition> ToParameterDefinition() override
    {
        return self();
    }
};

} // fraze
