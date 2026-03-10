/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/common/Object.h>
#include <fraze/ast/def/ClassDefinition.h>
#include <fraze/program/TypeInfo.h>
#include <memory>
#include <algorithm>

namespace fraze {

// ARRAY

Array<>* Array<>::New(IAllocator& allocator, const ArrayInfo* info, size_t length)
{
    auto allocSize = sizeof(Array<>) + sizeof(Word) * length;
    return Object::Create<Array<>>(allocator, allocSize, info, length);
}

Array<>::Array(const ArrayInfo* info, size_t length)
    : Object(info), length(length)
{
    Word* p = &GetWord(0);
    std::uninitialized_default_construct_n(p, length);
}

size_t Array<>::GetSize() const {
    return length;
}

size_t Array<>::GetElementSize() const {
    return GetInfo()->GetElementSize();
}

size_t Array<>::GetCount() const {
    return length / GetElementSize();
}

Word& Array<>::At(size_t index) {
    assert(index < length);
    return GetWord(index);
}

const Word& Array<>::At(size_t index) const {
    assert(index < length);
    return GetWord(index);
}

Word* Array<>::GetData() const {
    return (Word*)(this + 1);
}

Word& Array<>::GetWord(size_t i) const {
    return GetData()[i];
}

const ArrayInfo* Array<>::GetInfo() const
{
    return static_cast<const ArrayInfo*>(info);
}
// STRING

std::unique_ptr<String, Object::Deleter> String::New(Program* program, std::string_view str)
{
    auto size = sizeof(String) + str.size() * sizeof(char);
    TypeInfo* typeInfo = program->GetTypeInfo("string");
    assert(typeInfo);
    return Object::Create<String>(size, typeInfo, str);
}

String* String::New(IAllocator& allocator, std::string_view str)
{
    auto size = sizeof(String) + str.size() * sizeof(char);
    TypeInfo* typeInfo = allocator.program()->GetTypeInfo("string");
    assert(typeInfo);
    return Object::Create<String>(allocator, size, typeInfo, str);
}

String* String::New(IAllocator& allocator, size_t length)
{
    auto size = sizeof(String) + length * sizeof(char);
    TypeInfo* typeInfo = allocator.program()->GetTypeInfo("string");
    assert(typeInfo);
    return Object::Create<String>(allocator, size, typeInfo, length);
}

String* String::New(IAllocator& allocator, std::string_view left, std::string_view right)
{
    auto size = sizeof(String) + (left.size() + right.size()) * sizeof(char);
    TypeInfo* typeInfo = allocator.program()->GetTypeInfo("string");
    assert(typeInfo);
    return Object::Create<String>(allocator, size, typeInfo, left, right);
}

String::String(const TypeInfo* info, size_t length)
    : Object(info), length(length)
{
}

String::String(const TypeInfo* info, std::string_view str)
    : Object(info), length(str.size())
{
    if(!str.empty()) {
        memcpy(GetChar(0), str.data(), length);
    }
}

String::String(const TypeInfo* info, std::string_view left, std::string_view right)
    : Object(info)
{
    size_t i = 0;

    if(!left.empty()) {
        memcpy(GetChar(i), left.data(), left.size());
        i += left.size();
    }

    if(!right.empty()) {
        memcpy(GetChar(i), right.data(), right.size());
        i += right.size();
    }

    length = i;
}

std::string_view String::GetView() const
{
    return std::string_view(GetChar(0), length);
}

bool String::IsEmpty() const {
    return length == 0;
}

char* String::GetChar(size_t i) const {
    auto p = (char*)(this + 1);
    return p + i;
}

// CLASS 

Class* Class::New(IAllocator& allocator, const ClassInfo* info)
{
    assert(info);
    auto size = sizeof(Class) + sizeof(Word) * info->size;
    return Object::Create<Class>(allocator, size, info);
}

Class::Class(const ClassInfo* info)
    : Object(info)
{
    std::uninitialized_default_construct_n(&GetWord(0), info->size);
}

std::string_view Class::GetName() const {
    return info->qualifiedName;
}

size_t Class::GetFieldCount() const {
    return GetInfo()->size;
}

Word Class::GetField(size_t index) const {
    assert(index < GetInfo()->size);
    return GetWord(index);
}

Word Class::GetField(std::string_view name) const
{
    for(auto& field : GetInfo()->fields)
    {
        std::string_view sv = field->qualifiedName;
        if(size_t lastDot = sv.rfind("."))
            sv = sv.substr(lastDot + 1);

        if(sv == name)
        {
            return GetWord(field->offset);
        }
    }

    assert(0);
    return {};
}

Word Class::GetFieldRef(size_t index) const {
    assert(index < GetInfo()->size);
    return Word(&GetWord(index));
}

std::span<Word> Class::GetFields(size_t index, size_t size) const {
    assert(index < GetInfo()->size);
    assert(index + size <= GetInfo()->size);
    assert(size != 0);
    auto first = &GetWord(index);
    auto last = first + size;
    return { first, last };
}

void Class::SetField(size_t index, const Word& obj) {
    assert(index < GetInfo()->size);
    GetWord(index) = obj;
}

void Class::SetFields(size_t index, const std::span<Word>& values)
{
    auto fields = &GetWord(index);
    for(size_t i = 0; i != values.size(); ++i)
        fields[i] = values[i];
}

void Class::SetField(std::string_view name, const Word& val)
{
    for(auto& field : GetInfo()->fields)
    {
        std::string_view sv = field->qualifiedName;
        if(size_t lastDot = sv.rfind("."))
            sv = sv.substr(lastDot + 1);

        if(sv == name)
        {
            GetWord(field->offset) = val;
            return;
        }
    }

    assert(0);
}

void Class::SetField(std::string_view name, const std::span<Word>& val)
{
    for(auto& field : GetInfo()->fields)
    {
        std::string_view sv = field->qualifiedName;
        if(size_t lastDot = sv.rfind("."))
            sv = sv.substr(lastDot + 1);
        
        if(sv == name)
        {
            size_t argWordSize = val.size();
            size_t fieldWordSize = field->type->GetSize();
            assert(argWordSize == fieldWordSize);
            SetFields(field->offset, val);
            return;
        }
    }

    assert(0);
}

size_t Class::GetFunctionID(size_t interfaceID, size_t offset)
{
    for(size_t i = 0; i != GetInfo()->interfaces.size(); ++i)
    {
        if(GetInfo()->interfaces[i] == interfaceID)
        {
            return GetInfo()->implementations[i][offset];
        }
    }

    assert(0);
    return size_t(-1);
}

Word& Class::GetWord(size_t i) const {
    auto p = (Word*)(this + 1);
    return p[i];
}

const ClassInfo* Class::GetInfo() const {
    return static_cast<const ClassInfo*>(info);
}

} // fraze
