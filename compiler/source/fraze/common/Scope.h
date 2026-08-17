/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <iostream>
#include <ranges>
#include <string>
#include <unordered_map>
#include <vector>
#include <fraze/ast/ASTFwd.h>
#include <fraze/common/Exception.h>
#include <fraze/common/Pointers.h>
#include <fraze/common/Platform.h>

namespace fraze {

class ScopeStack;

class Scope
{
    static bool DefNamePredicate(const sptr<Definition>& def, std::string_view name);
public:
    Definition* owner{}; // enclosing definition
    std::vector<sptr<Definition>> definitions;
    Scope* parent{};

    void AddDefinition(const sptr<Definition>& def);

    // returns a definition with 'name', but only if exactly one exists.
    sptr<Definition> FindDefinition(std::string_view name);

    // returns a range of definitions with 'name'.
    auto FindDefinitions(std::string_view name);
    
    // searches upward for a definition with 'name'
    static sptr<Definition> SearchUpward(std::string_view name, sptr_view<Scope> fromScope, sptr_view<Scope> toScope);

    sptr<Definition> SearchUpward(std::string_view name, sptr_view<Scope> toScope = nullptr) {
        return SearchUpward(name, this, toScope);
    }

    Scope* GetRoot();

    sptr<Scope> Clone(ScopeStack& scopes);
};

inline auto Scope::FindDefinitions(std::string_view name)
{
    return definitions | std::views::filter([=](const auto& d) {
        return DefNamePredicate(d, name);
    });
}

class ScopeStack
{
    std::vector<Scope*> scopes;
public:
    void PushFromRoot(Scope* scope)
    {
        if(scope->parent)
            PushFromRoot(scope->parent);

        Push(scope);
    }

    void Push(Scope* scope) {
        scopes.push_back(scope);
    }

    void Pop() {
        scopes.pop_back();
    }

    void Clear() {
        scopes.clear();
    }

    Scope* GetCurrent() {
        return !scopes.empty() ? scopes.back() : nullptr;
    }
};

} // fraze
