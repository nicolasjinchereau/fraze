/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <fraze/common/Pointers.h>
#include <fraze/common/Exception.h>
#include <fraze/common/SourceLocation.h>
#include <fraze/common/Object.h>

namespace fraze
{

class IExternalFunction;

struct SectionInfo;
struct BasicTypeInfo;
struct FieldInfo;
struct FunctionInfo;
struct ClassInfo;
struct InterfaceInfo;
struct StructInfo;
struct EnumInfo;
struct ArrayInfo;

struct TypeInfo : public Object
{
    size_t id;
    std::string qualifiedName;
    SourceLocation loc;

    TypeInfo(const TypeInfo* typeInfo) : Object(typeInfo){}
    virtual ~TypeInfo(){}

    virtual SectionInfo* ToSectionInfo(){ return nullptr; }
    virtual BasicTypeInfo* ToBasicTypeInfo(){ return nullptr; }
    virtual FieldInfo* ToFieldInfo(){ return nullptr; }
    virtual FunctionInfo* ToFunctionInfo(){ return nullptr; }
    virtual ClassInfo* ToClassInfo(){ return nullptr; }
    virtual InterfaceInfo* ToInterfaceInfo(){ return nullptr; }
    virtual StructInfo* ToStructInfo(){ return nullptr; }
    virtual EnumInfo* ToEnumInfo(){ return nullptr; }
    virtual ArrayInfo* ToArrayInfo(){ return nullptr; }

    virtual const SectionInfo* ToSectionInfo() const { return nullptr; }
    virtual const BasicTypeInfo* ToBasicTypeInfo() const { return nullptr; }
    virtual const FieldInfo* ToFieldInfo() const { return nullptr; }
    virtual const FunctionInfo* ToFunctionInfo() const { return nullptr; }
    virtual const ClassInfo* ToClassInfo() const { return nullptr; }
    virtual const InterfaceInfo* ToInterfaceInfo() const { return nullptr; }
    virtual const StructInfo* ToStructInfo() const { return nullptr; }
    virtual const EnumInfo* ToEnumInfo() const { return nullptr; }
    virtual const ArrayInfo* ToArrayInfo() const { return nullptr; }

    virtual size_t GetSize() const { return 1; }
};

struct SectionInfo : TypeInfo
{
    uint32_t codeStart;
    uint32_t codeEnd;

    SectionInfo(const TypeInfo* typeInfo) : TypeInfo(typeInfo) {}

    virtual SectionInfo* ToSectionInfo() override { return this; }
    virtual const SectionInfo* ToSectionInfo() const override { return this; }
};

struct BasicTypeInfo : TypeInfo
{
    BasicTypeInfo(const TypeInfo* typeInfo) : TypeInfo(typeInfo) {}

    virtual BasicTypeInfo* ToBasicTypeInfo() override { return this; }
    virtual const BasicTypeInfo* ToBasicTypeInfo() const override { return this; }
};

struct FieldInfo : TypeInfo
{
    TypeInfo* type{};
    size_t offset;

    FieldInfo(const TypeInfo* typeInfo) : TypeInfo(typeInfo) {}

    virtual FieldInfo* ToFieldInfo() override { return this; }
    virtual const FieldInfo* ToFieldInfo() const override { return this; }
};

struct ParamInfo
{
    uint32_t offset;
    uint32_t size;
};

struct FunctionInfo : TypeInfo
{
    uint32_t returnSize;
    uint32_t paramSize;
    uint32_t localSize;
    uint32_t codeStart;
    uint32_t codeEnd;
    uint32_t offset;
    sptr<IExternalFunction> externalFunction;
    int intrinsicID;
    bool hasContext;
    bool isExternal;
    std::vector<ParamInfo> params;

    FunctionInfo(const TypeInfo* typeInfo) : TypeInfo(typeInfo) {}

    virtual FunctionInfo* ToFunctionInfo() override { return this; }
    virtual const FunctionInfo* ToFunctionInfo() const override { return this; }
};

struct ClassInfo : TypeInfo
{
    size_t size{};
    std::vector<size_t> interfaces;
    std::vector<std::vector<size_t>> implementations;
    std::vector<FieldInfo*> fields;
    bool isExternal{};

    ClassInfo(const TypeInfo* typeInfo) : TypeInfo(typeInfo) {}

    virtual ClassInfo* ToClassInfo() override { return this; }
    virtual const ClassInfo* ToClassInfo() const override { return this; }
};

struct InterfaceInfo : TypeInfo
{
    InterfaceInfo(const TypeInfo* typeInfo) : TypeInfo(typeInfo) {}

    virtual InterfaceInfo* ToInterfaceInfo() override { return this; }
    virtual const InterfaceInfo* ToInterfaceInfo() const override { return this; }
};

struct StructInfo : TypeInfo
{
    size_t size{};
    std::vector<FieldInfo*> fields;

    StructInfo(const TypeInfo* typeInfo) : TypeInfo(typeInfo) {}

    virtual StructInfo* ToStructInfo() override { return this; }
    virtual const StructInfo* ToStructInfo() const override { return this; }

    virtual size_t GetSize() const override { return size; }
};

struct EnumInfo : TypeInfo
{
    std::vector<std::pair<std::string, int64_t>> members;

    EnumInfo(const TypeInfo* typeInfo) : TypeInfo(typeInfo) {}

    virtual EnumInfo* ToEnumInfo() override { return this; }
    virtual const EnumInfo* ToEnumInfo() const override { return this; }

    virtual size_t GetSize() const override { return 1; }
};

struct ArrayInfo : TypeInfo
{
    TypeInfo* elementType{};

    ArrayInfo(const TypeInfo* typeInfo) : TypeInfo(typeInfo) {}

    virtual ArrayInfo* ToArrayInfo() override { return this; }
    virtual const ArrayInfo* ToArrayInfo() const override { return this; }

    size_t GetElementSize() const { return elementType->GetSize(); }
};

} // fraze
