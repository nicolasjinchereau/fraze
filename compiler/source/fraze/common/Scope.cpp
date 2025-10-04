/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <iostream>
#include <fraze/ast/def/Definition.h>
#include <fraze/common/Pointers.h>
#include <fraze/ast/AST.h>
#include <ranges>

namespace fraze
{

void Scope::AddDefinition(const sptr<Definition>& def)
{
    auto it = std::ranges::find_if(definitions, [&](const sptr<Definition>& d){
        return d->name == def->name;
    });

    ENFORCE(!!def->ToFunctionDefinition() || it == definitions.end(),
        def->loc, "multiple definitions found");

    definitions.push_back(def);

    if(def->parent)
        def->parent->children.push_back(def);
}

sptr<Definition> Scope::FindDefinition(std::string_view name)
{
    sptr<Definition> ret;

    for(auto& def : definitions)
    {
        if(def->name == name)
        {
            if(!ret)
            {
                ret = def;
            }
            else
            {
                // found multiple, return null
                ret = nullptr;
                break;
            }
        }

    }

    return ret;
}

sptr<Definition> Scope::SearchUpward(std::string_view name, Scope* toScope)
{
    sptr<Definition> ret;

    for(auto sc = this; sc != nullptr; sc = sc->parent)
    {
        ret = sc->FindDefinition(name);
        if(ret)
            break;

        if(sc == toScope)
            break;
    }

    return ret;
}

Scope* Scope::GetRoot()
{
    auto root = this;

    while(root->parent)
        root = root->parent;

    return root;
}

sptr<Scope> Scope::Clone(ScopeStack& scopes)
{
    auto copy = spnew<Scope>();

    for(auto& def : definitions)
    {
        auto defCopy = def->Clone(scopes, nullptr)->ToDefinition();
        copy->definitions.push_back(defCopy);
    }

    return copy;
}

bool Scope::DefNamePredicate(const sptr<Definition>& def, std::string_view name)
{
    return def->name == name;
}

} // fraze
