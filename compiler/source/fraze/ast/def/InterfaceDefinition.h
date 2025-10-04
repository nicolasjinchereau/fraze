/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/def/VariableDefinition.h>
#include <fraze/ast/def/FunctionDefinition.h>
#include <fraze/ast/def/TemplateParameterDefinition.h>
#include <fraze/ast/def/TemplateDefinition.h>
#include <fraze/ast/type/Type.h>
#include <fraze/common/Object.h>
#include <fraze/common/Scope.h>

namespace fraze {

class InterfaceDefinition : public TemplateDefinition
{
public:
    Type* type{};
    bool isFunctor = false;

    InterfaceDefinition(
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

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copyName = templateType ? templateType->GetElementTypeName() : name;
        auto copyArgs = templateType ? templateType->templateArgs : decltype(templateArgs){};
        auto copy = spnew<InterfaceDefinition>(loc, scopes.GetCurrent(), copyName, copyArgs);

        scopes.GetCurrent()->AddDefinition(copy);

        copy->isFunctor = isFunctor;

        scopes.Push(copy->scope.get());

        for(auto& def : scope->definitions)
            def->Clone(scopes, nullptr);

        scopes.Pop();

        copy->ApplyTemplateArgs();

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<InterfaceDefinition> ToInterfaceDefinition() override
    {
        return self();
    }

    virtual sptr<InterfaceDefinition> ToFunctorInterfaceDefinition() override
    {
        return isFunctor ? self() : sptr<InterfaceDefinition>{};
    }
};

} // fraze
