/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/common/Object.h>
#include <fraze/ast/def/ClassDefinition.h>
#include <memory>
#include <algorithm>

namespace fraze {

// ARRAY

Array<>* Array<>::New(Heap& heap, const ArrayInfo* info, size_t length)
{
    auto allocSize = sizeof(Array<>) + sizeof(Word) * length;
    return Object::Create<Array<>>(heap, allocSize, info, length);
}

Array<>::Array(const ArrayInfo* info, size_t length)
    : info(info), length(length)
{
    Word* p = &GetWord(0);
    std::uninitialized_default_construct_n(p, length);
}

Array<>::~Array() {
    std::destroy_n(&GetWord(0), length);
}

size_t Array<>::GetSize() const {
    return length;
}

size_t Array<>::GetElementSize() const {
    return info->GetElementSize();
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

// STRING

std::unique_ptr<String, Object::Deleter> String::New(std::string_view str)
{
    auto size = sizeof(String) + str.size() * sizeof(char);
    return Object::Create<String>(size, str);
}

String* String::New(Heap& heap, std::string_view str)
{
    auto size = sizeof(String) + str.size() * sizeof(char);
    return Object::Create<String>(heap, size, str);
}

String* String::New(Heap& heap, size_t length)
{
    auto size = sizeof(String) + length * sizeof(char);
    return Object::Create<String>(heap, size, length);
}

String* String::New(Heap& heap, std::string_view left, std::string_view right)
{
    auto size = sizeof(String) + (left.size() + right.size()) * sizeof(char);
    return Object::Create<String>(heap, size, left, right);
}

String::String(size_t length)
    : length(length)
{
}

String::String(std::string_view str)
    : length(str.size())
{
    if(!str.empty()) {
        memcpy(GetChar(0), str.data(), length);
    }
}

String::String(std::string_view left, std::string_view right)
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

Class* Class::New(Heap& heap, const ClassInfo* info)
{
    assert(info);
    auto size = sizeof(Class) + sizeof(Word) * info->size;
    return Object::Create<Class>(heap, size, info);
}

Class::Class(const ClassInfo* info)
    : info(info)
{
    std::uninitialized_default_construct_n(&GetWord(0), info->size);
}

Class::~Class() {
    std::destroy_n(&GetWord(0), info->size);
}

std::string_view Class::GetName() const {
    return info->qualifiedName;
}

size_t Class::GetFieldCount() const {
    return info->size;
}

Word Class::GetField(size_t index) const {
    assert(index < info->size);
    return GetWord(index);
}

Word Class::GetField(std::string_view name) const
{
    for(auto& field : info->fields)
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
    assert(index < info->size);
    return Word(&GetWord(index));
}

std::span<Word> Class::GetFields(size_t index, size_t size) const {
    assert(index < info->size);
    assert(index + size <= info->size);
    assert(size != 0);
    auto first = &GetWord(index);
    auto last = first + size;
    return { first, last };
}

void Class::SetField(size_t index, const Word& obj) {
    assert(index < info->size);
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
    for(auto& field : info->fields)
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
    for(auto& field : info->fields)
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
    for(size_t i = 0; i != info->interfaces.size(); ++i)
    {
        if(info->interfaces[i] == interfaceID)
        {
            return info->implementations[i][offset];
        }
    }

    assert(0);
    return size_t(-1);
}

Word& Class::GetWord(size_t i) const {
    auto p = (Word*)(this + 1);
    return p[i];
}

// BOX

Box* Box::New(Heap& heap, Word value, WordType type) {
    return Object::Create<Box>(heap, sizeof(Box), value, type);
}

} // fraze
