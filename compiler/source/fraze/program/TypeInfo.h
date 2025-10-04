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

struct TypeInfo
{
    size_t id;
    std::string qualifiedName;
    SourceLocation loc;

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

    virtual size_t GetSize() const { return 1; }
};

struct SectionInfo : TypeInfo
{
    uint32_t codeStart;
    uint32_t codeEnd;

    virtual SectionInfo* ToSectionInfo() override { return this; }
};

struct BasicTypeInfo : TypeInfo
{
    //WordType type;
    virtual BasicTypeInfo* ToBasicTypeInfo() override { return this; }
};

struct FieldInfo : TypeInfo
{
    TypeInfo* type{};
    size_t offset;

    virtual FieldInfo* ToFieldInfo() override { return this; }
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
    std::vector<std::pair<int, int>> params;

    virtual FunctionInfo* ToFunctionInfo() override { return this; }
};

struct ClassInfo : TypeInfo
{
    size_t size{};
    std::vector<size_t> interfaces;
    std::vector<std::vector<size_t>> implementations;
    std::vector<FieldInfo*> fields;
    virtual ClassInfo* ToClassInfo() override { return this; }
};

struct InterfaceInfo : TypeInfo
{
    virtual InterfaceInfo* ToInterfaceInfo() override { return this; }
};

struct StructInfo : TypeInfo
{
    size_t size{};
    std::vector<FieldInfo*> fields;
    virtual StructInfo* ToStructInfo() override { return this; }

    virtual size_t GetSize() const override { return size; }
};

struct EnumInfo : TypeInfo
{
    std::vector<std::pair<std::string, int64_t>> members;

    virtual EnumInfo* ToEnumInfo() override { return this; }
    virtual size_t GetSize() const override { return 1; }
};

struct ArrayInfo : TypeInfo
{
    TypeInfo* elementType{};

    virtual ArrayInfo* ToArrayInfo() override { return this; }

    size_t GetElementSize() const { return elementType->GetSize(); }
};

} // fraze
