/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <string>
#include <iostream>
#include <ranges>
#include <fraze/ast/ASTNode.h>
#include <fraze/common/Extensions.h>
#include <fraze/common/Object.h>
#include <fraze/common/Pointers.h>
#include <fraze/common/Scope.h>
#include <fraze/common/SharedString.h>

namespace fraze {

class ASTRoot;

class Definition : public ASTNode
{
    friend ASTRoot;
    ASTRoot* astRoot{};
public:
    Scope* enclosingScope{};
    Definition* parent{};
    std::vector<sptr<Definition>> children; // populated by Scope::AddDefinition
    shared_string name; // the name introduced by this definition
    shared_string qualifiedName;
    sptr<Scope> scope; // scope created by this section/class/function

    Definition(const SourceLocation& loc, Scope* enclosingScope, const shared_string& name)
        : ASTNode(loc), enclosingScope(enclosingScope), name(name)
    {
        assert(!name.empty());
        assert((enclosingScope && enclosingScope->owner) || name == "global");
        
        parent = enclosingScope ? enclosingScope->owner : nullptr;
        qualifiedName = GetQualifiedName();
    }
    
    ASTRoot* GetRoot()
    {
        auto node = this;

        while(node->parent)
            node = node->parent;

        return node->astRoot;
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        ENFORCE(false, loc, "Not supported");
        return {};
    }

    virtual void Accept(ASTVisitor& visitor) override = 0;

    virtual sptr<Definition> ToDefinition() override {
        return self();
    }

    bool IsGlobal() const {
        return parent == nullptr;
    }

    virtual bool IsPrivate() const {
        return false;
    }

    bool IsAccessibleFrom(const Scope* scope) const;

    bool IsPartOfTemplateDeclaration();

    sptr<FunctionDefinition> GetFunction(std::string_view name)
    {
        auto def = scope->FindDefinition(name);
        return def ? def->ToFunctionDefinition() : sptr<FunctionDefinition>{};
    }

    sptr<VariableDefinition> GetVariable(std::string_view name)
    {
        auto def = scope->FindDefinition(name);
        return def ? def->ToVariableDefinition() : sptr<VariableDefinition>{};
    }

    template<class T>
    auto GetChildren() const;

    template<class T>
    auto GetChildren(std::string_view name) const;

    template<class T>
    auto GetFirstChild() const;

    template<class T>
    auto GetFirstChild(std::string_view name) const;
private:
    shared_string GetQualifiedName()
    {
        std::string qualifiedName;

        if(parent)
        {
            qualifiedName = parent->GetQualifiedName().str();

            if(!qualifiedName.empty())
                qualifiedName += ".";

            qualifiedName += name.str();
        }

        return shared_string{ qualifiedName };
    }
};

template<class T> requires std::derived_from<T, Definition>
inline sptr<T> DefinitionCast(const sptr<Definition>& def)
{
    if constexpr(std::is_same_v<T, Definition>)
        return def;
    else if constexpr(std::is_same_v<T, BasicTypeDefinition>)
        return def->ToBasicTypeDefinition();
    else if constexpr(std::is_same_v<T, ClassDefinition>)
        return def->ToClassDefinition();
    else if constexpr(std::is_same_v<T, FunctionDefinition>)
        return def->ToFunctionDefinition();
    else if constexpr(std::is_same_v<T, InterfaceDefinition>)
        return def->ToInterfaceDefinition();
    else if constexpr(std::is_same_v<T, ParameterDefinition>)
        return def->ToParameterDefinition();
    else if constexpr(std::is_same_v<T, PropertyDefinition>)
        return def->ToPropertyDefinition();
    else if constexpr(std::is_same_v<T, SectionDefinition>)
        return def->ToSectionDefinition();
    else if constexpr(std::is_same_v<T, StructDefinition>)
        return def->ToStructDefinition();
    else if constexpr(std::is_same_v<T, TemplateDefinition>)
        return def->ToTemplateDefinition();
    else if constexpr(std::is_same_v<T, TemplateParameterDefinition>)
        return def->ToTemplateParameterDefinition();
    else if constexpr(std::is_same_v<T, VariableDefinition>)
        return def->ToVariableDefinition();
    else if constexpr(std::is_same_v<T, EnumDefinition>)
        return def->ToEnumDefinition();
    else if constexpr(std::is_same_v<T, EnumMemberDefinition>)
        return def->ToEnumMemberDefinition();
    else
        static_assert(false, "Unsupported Definition Type");
}

template<class T>
inline auto Definition::GetChildren() const
{
    return children
        | std::views::transform([](const sptr<Definition>& d) { return DefinitionCast<T>(d); })
        | std::views::filter([](const sptr<T>& p) { return p != nullptr; })
        | countable;
}

template<class T>
inline auto Definition::GetChildren(std::string_view name) const
{
    return children
        | std::views::transform([](const sptr<Definition>& d) { return DefinitionCast<T>(d); })
        | std::views::filter([=](const sptr<T>& p) { return p != nullptr && p->name == name; })
        | countable;
}

template<class T>
inline auto Definition::GetFirstChild() const
{
    auto r = children
        | std::views::transform([](const sptr<Definition>& d) { return DefinitionCast<T>(d); })
        | std::views::filter([](const sptr<T>& p) { return p != nullptr; });
    return !r.empty() ? r.front() : sptr<T>{};
}

template<class T>
inline auto Definition::GetFirstChild(std::string_view name) const
{
    auto r = children
        | std::views::transform([](const sptr<Definition>& d) { return DefinitionCast<T>(d); })
        | std::views::filter([=](const sptr<T>& p) { return p != nullptr && p->name == name; });
    return !r.empty() ? r.front() : sptr<T>{};
}

} // fraze
