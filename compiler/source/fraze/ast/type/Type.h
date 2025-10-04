/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <string>
#include <fraze/common/Pointers.h>

namespace fraze {

class Definition;
class TemplateDefinition;

class Type
{
    shared_string name;
    Definition* def;
    std::vector<Type*> templateArgs; // for templates
    Type* elementType; // for arrays

    void Init(const shared_string& name, Definition* def);
    void Init(const shared_string& name, TemplateDefinition* def);
    void Init(const shared_string& name, Type* elementType);

    static bool Create(Definition* def, sptr<Type> &type);
public:

    static Type* Get(Definition* def);
    static Type* Get(TemplateDefinition* def);
    static Type* Get(Type* arrayElementType);
    static Type* Get(std::string_view typeName);

    const shared_string& GetName() const {
        return name;
    }

    shared_string GetBaseTypeName() const
    {
        std::string_view ret = name;
        
        size_t baseNameEnd = ret.find_last_not_of("[]");
        if(baseNameEnd != std::string::npos)
            ret = ret.substr(0, baseNameEnd + 1);

        return shared_string(ret);
    }

    int GetArrayDimensions() const
    {
        int dimensions = 0;

        size_t baseNameEnd = name.find_last_not_of("[]");
        if(baseNameEnd != std::string::npos)
            dimensions = static_cast<int>((name.size() - (baseNameEnd + 1)) / 2);

        return dimensions;
    }

    const std::vector<Type*>& GetTemplateArgs() const {
        return templateArgs;
    }

    bool IsPlaceholder() const {
        return name == "$placeholder";
    }

    bool IsVoid() const {
        return this == Type::Get("void");
    }

    bool IsNull() const {
        return this == Type::Get("null");
    }

    bool IsObject() const {
        return this == Type::Get("object");
    }

    bool IsArray() const {
        return !!elementType;
    }

    bool IsBoolean() const {
        return this == Type::Get("bool");
    }

    bool IsInteger() const {
        return this == Type::Get("int");
    }

    bool IsNumber() const {
        return this == Type::Get("num");
    }

    bool IsFunction() const {
        return def && (def->ToFunctionDefinition() != nullptr);
    }

    bool IsFunctorInterface() const {
        return def && (def->ToFunctorInterfaceDefinition() != nullptr);
    }

    bool IsFunctorClass() const {
        return def && (def->ToFunctorClassDefinition() != nullptr);
    }
    
    bool IsClass() const {
        return def && (def->ToClassDefinition() != nullptr);
    }

    bool IsStruct() const {
        return def && (def->ToStructDefinition() != nullptr);
    }

    bool IsEnum() const {
        return def && (def->ToEnumDefinition() != nullptr);
    }

    bool IsInterface() const {
        return def && (def->ToInterfaceDefinition() != nullptr);
    }

    bool IsString() const {
        return def && def->qualifiedName == "string";
    }

    bool IsNullable() const {
        return IsObject() || IsArray() || IsClass() || IsInterface() || IsString();
    }

    Definition* GetDefinition() {
        return def;
    }

    Type* GetElementType() {
        assert(elementType);
        return elementType;
    }

    Type* ArrayOf() {
        Type* arrayElementType = this;
        return Get(arrayElementType);
    }
};

} // fraze
