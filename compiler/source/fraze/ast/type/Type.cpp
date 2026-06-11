/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/AST.h>
#include <fraze/ast/type/Type.h>
#include <fraze/ast/def/TemplateDefinition.h>
#include <fraze/common/Exception.h>
#include <fraze/compiler/Compiler.h>

namespace fraze {

void Type::Init(const shared_string& name, Definition* def)
{
    this->name = name;
    this->def = def;
    this->elementType = nullptr;

    if(auto templateDef = def->ToTemplateDefinition())
    {
        for(auto& arg : templateDef->templateArgs)
        {
            assert(arg->type);
            this->templateArgs.push_back(arg->type);
        }
    }
}

void Type::Init(const shared_string& name, TemplateDefinition* def)
{
    this->name = name;
    this->def = def;
    this->elementType = nullptr;

    for(auto& arg : def->templateArgs)
    {
        assert(arg->type);
        this->templateArgs.push_back(arg->type);
    }
}

void Type::Init(const shared_string& name, Type* elementType)
{
    this->name = name;
    this->def = nullptr;
    this->elementType = elementType;
}

bool Type::Create(Definition* def, sptr<Type>& type)
{
    assert(def);
    assert(!def->qualifiedName.empty() || def->name == "global");

    auto compiler = Compiler::GetActiveCompiler();
    if(!compiler)
        throw Exception("no active compiler");

    auto it = std::find_if(
        compiler->types.begin(), compiler->types.end(),
        [def](const sptr<Type>& ty) {
            return ty->GetDefinition() == def;
        });

    if(it != compiler->types.end())
    {
        type = *it;
        return false;
    }

    type = spnew<Type>();
    compiler->types.push_back(type);
    return true;
}

Type* Type::Get(Definition* def)
{
    sptr<Type> type;
    if(Create(def, type))
    {
        type->Init(def->qualifiedName, def);
    }

    return type.get();
}

Type* Type::Get(TemplateDefinition* def)
{
    sptr<Type> type;
    if(Create(def, type))
    {
        type->Init(def->qualifiedName, def);
    }

    return type.get();
}

Type* Type::Get(Type* arrayElementType)
{
    assert(arrayElementType);

    auto compiler = Compiler::GetActiveCompiler();
    if(!compiler)
        throw Exception("no active compiler");

    auto it = std::find_if(
        compiler->types.begin(), compiler->types.end(),
        [arrayElementType](const sptr<Type>& ty) {
            return ty->IsArray() && ty->GetElementType() == arrayElementType;
        });

    if(it != compiler->types.end())
        return it->get();

    sptr<Type> type = spnew<Type>();
    type->Init(shared_string(arrayElementType->name + "[]"), arrayElementType);
    compiler->types.push_back(type);

    return type.get();
}

Type* Type::Get(std::string_view typeName)
{
    Type* type{};

    if(auto compiler = Compiler::GetActiveCompiler())
    {
        auto it = std::find_if(
            compiler->types.begin(), compiler->types.end(),
            [&](const sptr<Type>& ty) {
                return ty->GetName() == typeName;
            });

        if(it != compiler->types.end())
        {
            return it->get();
        }
    }

    return type;
}

bool Type::IsFunction() const
{
    return def && (def->ToFunctionDefinition() != nullptr);
}

bool Type::IsFunctorInterface() const
{
    return def && (def->ToFunctorInterfaceDefinition() != nullptr);
}

bool Type::IsFunctorClass() const
{
    return def && (def->ToFunctorClassDefinition() != nullptr);
}

bool Type::IsClass() const
{
    return def && (def->ToClassDefinition() != nullptr);
}

bool Type::IsStruct() const
{
    return def && (def->ToStructDefinition() != nullptr);
}

bool Type::IsEnum() const
{
    return def && (def->ToEnumDefinition() != nullptr);
}

bool Type::IsInterface() const
{
    return def && (def->ToInterfaceDefinition() != nullptr);
}

bool Type::IsString() const
{
    return def && def->qualifiedName == "string";
}

} // fraze
