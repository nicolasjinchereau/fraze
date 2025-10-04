/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/def/TemplateParameterDefinition.h>
#include <fraze/ast/type/Type.h>
#include <fraze/ast/type/TypeSpecifier.h>
#include <fraze/common/Object.h>
#include <fraze/common/Scope.h>
#include <string>
#include <vector>

namespace fraze {

class TemplateDefinition : public Definition
{
public:
    std::vector<sptr<TypeSpecifier>> templateArgs;

    TemplateDefinition(const SourceLocation& loc, Scope* enclosingScope, const shared_string& name, const std::vector<sptr<TypeSpecifier>>& templateArgs)
        : Definition(loc, enclosingScope, name), templateArgs(templateArgs)
    {
    }

    bool IsTemplateDeclaration() const {
        auto tempParams = GetChildren<TemplateParameterDefinition>();
        return !tempParams.empty() && templateArgs.empty();
    }

    bool IsTemplateInstance() const {
        return !templateArgs.empty();
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        ENFORCE(false, loc, "Not supported");
        return {};
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<TemplateDefinition> ToTemplateDefinition() override
    {
        return self();
    }

protected:
    void ApplyTemplateArgs();
};

} // fraze
