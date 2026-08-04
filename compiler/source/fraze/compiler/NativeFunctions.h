/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/common/Object.h>
#include <fraze/common/Platform.h>
#include <fraze/memory/ScopedAllocator.h>
#include <fraze/program/Program.h>
#include <fraze/program/Dispatcher.h>
#include <cmath>
#include <print>

namespace fraze {

// MATH

inline Number Math_Fmod(const Number& x, const Number& y) {
    return std::fmod(x, y);
}

inline Number Math_Abs(const Number& value) {
    return std::abs(value);
}

inline Number Math_Sqrt(const Number& value) {
    return std::sqrt(value);
}

inline Number Math_Sin(const Number& value) {
    return std::sin(value);
}

inline Number Math_Cos(const Number& value) {
    return std::cos(value);
}

inline Number Math_Tan(const Number& value) {
    return std::tan(value);
}

inline Number Math_Asin(const Number& value) {
    return std::asin(value);
}

inline Number Math_Acos(const Number& value) {
    return std::acos(value);
}

inline Number Math_Atan(const Number& value) {
    return std::atan(value);
}

inline Number Math_Atan2(const Number& y, const Number& x) {
    return std::atan2(y, x);
}

// SYSTEM

inline void GC_Collect(Program* program) {
    program->Collect();
}

inline void GC_Report(Program* program) {
    program->Report();
}

inline void Console_Write(const String& text) {
    std::print("{}", text.GetView());
}

inline void Console_WriteLine(const String& text) {
    std::println("{}", text.GetView());
}

inline Integer Boolean_GetHashCode(const Boolean& value) {
    return value ? 1 : 0;
}

inline Integer Integer_GetHashCode(const Integer& value) {
    return value;
}

inline Integer Number_GetHashCode(const Number& value) {
    return std::bit_cast<Integer>(value);
}

inline void WaitAsync(Program* program, Class& task, const Number& seconds)
{
    program->PinMemory(&task);

    auto fseconds = std::chrono::duration<float>(seconds);
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(fseconds);
    auto resumeTime = std::chrono::steady_clock::now() + millis;

    Dispatcher::GetCurrent()->InvokeAsync([&, program]{
        task.SetField("$position", Integer(-1));
        program->Invoke("OnAwaitableCompleted", &task);
        program->UnpinMemory(&task);
    }, resumeTime);
}

inline void YieldAsync(Program* program, Class& task)
{
    program->PinMemory(&task);

    Dispatcher::GetCurrent()->InvokeAsync([&, program]{
        task.SetField("$position", Integer(-1));
        program->Invoke("OnAwaitableCompleted", &task);
        program->UnpinMemory(&task);
    });
}

inline Array<String>* String_Split(Program* program, const String& input, const String& delim)
{
    fraze::ScopedAllocator alloc(program);

    std::string_view str = input.GetView();
    std::string_view del = delim.GetView();

    std::vector<std::string_view> parts;

    size_t a = 0;
    size_t b = str.find(del, a);

    while(b != std::string::npos)
    {
        if(b - a != 0)
            parts.push_back(std::string_view(str.begin() + a, str.begin() + b));

        a = b + del.size();
        b = str.find(del, a);
    }

    if(a != str.size())
        parts.push_back(std::string_view(str.begin() + a, str.end()));

    Array<String>* ret = NEW_FRAZE_ARRAY_T(alloc, String, "string[]", parts.size());

    for(size_t i = 0; i != parts.size(); ++i)
    {
        ret->At(i) = NEW_FRAZE_STRING(alloc, parts[i]);
    }

    return ret;
}

inline Integer String_GetHashCode(const String& value)
{
    Integer hash = 5381;
    auto str = value.GetView();

    for(int i = 0; i < str.size(); i++)
        hash = ((hash << 5) + hash) ^ str[i];

    return hash;
}

inline String* String_Concat(Program* program, const String& left, const String& right)
{
    ScopedAllocator alloc(program);
    return NEW_FRAZE_STRING_JOIN(alloc, left.GetView(), right.GetView());
}

inline Boolean String_Equals(const String* left, const String* right)
{
    return left == right || (left && right && left->GetView() == right->GetView());
}

inline String* String_FromBool(Program* program, const Boolean& value)
{
    ScopedAllocator alloc(program);
    return NEW_FRAZE_STRING(alloc, value ? "true" : "false");
}

inline String* String_FromInt(Program* program, const Integer& value)
{
    std::array<char, 32> buffer;
    auto ret = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value);
    assert(ret.ec == std::errc());

    ScopedAllocator alloc(program);
    return NEW_FRAZE_STRING(alloc, std::string_view(buffer.data(), ret.ptr));
}

inline String* String_FromNum(Program* program, const Number& value)
{
    std::array<char, 1080> buffer;
    auto ret = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, std::chars_format::fixed);
    assert(ret.ec == std::errc());

    ScopedAllocator alloc(program);
    return NEW_FRAZE_STRING(alloc, std::string_view(buffer.data(), ret.ptr));
}

inline String* String_FromEnum(Program* program, const TypeInfo& enumTypeInfo, const Integer& value)
{
    const EnumInfo* enumInfo = enumTypeInfo.ToEnumInfo();
    auto it = std::ranges::find_if(enumInfo->members, [&](auto m) { return m.second == value; });
    assert(it != enumInfo->members.end());

    ScopedAllocator alloc(program);
    return NEW_FRAZE_STRING(alloc, std::string_view(it->first));
}

inline Integer Object_GetHashCode(const Object& value)
{
    union {
        const Object* object;
        Integer integer;
    } u;

    u.object = &value;
    return u.integer;
}

inline Object* Type_Find(Program* program, const Integer& typeID)
{
    return program->typeInfo[typeID].get();
}

inline String* Type_GetName(Program* program, const TypeInfo& self)
{
    ScopedAllocator alloc(program);
    return NEW_FRAZE_STRING(alloc, self.qualifiedName);
}

inline Boolean Type_IsInstance(const Object* obj, const TypeInfo& rightTypeInfo)
{
    bool isInstance = false;

    if (obj == nullptr)
    {
        if (auto bt = rightTypeInfo.ToBasicTypeInfo(); bt && bt->qualifiedName == "null")
            isInstance = true;

        return isInstance;
    }

    const TypeInfo* leftTypeInfo = obj->GetTypeInfo();

    if (auto leftClassInfo = leftTypeInfo->ToClassInfo()) // class or interface
    {
        if (leftClassInfo->qualifiedName == "Boolean")
        {
            if (auto bt = rightTypeInfo.ToBasicTypeInfo(); bt && bt->qualifiedName == "bool")
                isInstance = true;
            else if (auto ci = rightTypeInfo.ToClassInfo(); ci && ci->qualifiedName == "Boolean")
                isInstance = true;
        }
        else if (leftClassInfo->qualifiedName == "Integer")
        {
            if (auto bt = rightTypeInfo.ToBasicTypeInfo(); bt && bt->qualifiedName == "int")
                isInstance = true;
            else if (auto ci = rightTypeInfo.ToClassInfo(); ci && ci->qualifiedName == "Integer")
                isInstance = true;
        }
        else if (leftClassInfo->qualifiedName == "Number")
        {
            if (auto bt = rightTypeInfo.ToBasicTypeInfo(); bt && bt->qualifiedName == "num")
                isInstance = true;
            else if (auto ci = rightTypeInfo.ToClassInfo(); ci && ci->qualifiedName == "Number")
                isInstance = true;
        }
        else if (auto rightClassInfo = rightTypeInfo.ToClassInfo())
        {
            if (leftClassInfo->id == rightClassInfo->id)
                isInstance = true;
        }
        else if (auto rightInterfaceInfo = rightTypeInfo.ToInterfaceInfo())
        {
            for (auto& itf : leftClassInfo->interfaces)
            {
                if (itf == rightInterfaceInfo->id)
                {
                    isInstance = true;
                    break;
                }
            }
        }
    }
    else if (auto leftArrayInfo = leftTypeInfo->ToArrayInfo())
    {
        if (auto rightArrInfo = rightTypeInfo.ToArrayInfo())
        {
            if (leftArrayInfo->id == rightArrInfo->id)
                isInstance = true;
        }
    }
    else if (auto leftBasicTypeInfo = leftTypeInfo->ToBasicTypeInfo();
        leftBasicTypeInfo && leftBasicTypeInfo->qualifiedName == "string")
    {
        if (auto bt = rightTypeInfo.ToBasicTypeInfo(); bt && bt->qualifiedName == "string")
            isInstance = true;
    }

    return isInstance;
}

inline Object* Type_AsInstance(const Object* obj, const TypeInfo& rightTypeInfo)
{
    auto leftTypeInfo = obj->GetTypeInfo();

    if (auto leftClassInfo = leftTypeInfo->ToClassInfo())
    {
        if (auto rightClassInfo = rightTypeInfo.ToClassInfo())
        {
            if (leftClassInfo->id != rightClassInfo->id)
                obj = nullptr;
        }
        else if (auto rightInterfaceInfo = rightTypeInfo.ToInterfaceInfo())
        {
            bool match = false;

            for (auto& implementedItf : leftClassInfo->interfaces)
            {
                if (implementedItf == rightInterfaceInfo->id)
                {
                    match = true;
                    break;
                }
            }

            if (!match)
                obj = nullptr;
        }
    }
    else if (auto leftArrayInfo = leftTypeInfo->ToArrayInfo())
    {
        if (auto rightArrayInfo = rightTypeInfo.ToArrayInfo())
        {
            if (leftArrayInfo->id != rightArrayInfo->id)
                obj = nullptr;
        }
        else if (auto rightBasicTypeInfo = rightTypeInfo.ToBasicTypeInfo())
        {
            if(rightBasicTypeInfo->qualifiedName != "object")
                obj = nullptr;
        }
        else
        {
            obj = nullptr;
        }
    }
    else if (auto leftBasicTypeInfo = leftTypeInfo->ToBasicTypeInfo();
        leftBasicTypeInfo && leftBasicTypeInfo->qualifiedName == "string")
    {
        if(auto rightBasicTypeInfo = rightTypeInfo.ToBasicTypeInfo())
        {
            if(rightBasicTypeInfo->qualifiedName != "string" && rightBasicTypeInfo->qualifiedName != "object")
                obj = nullptr;
        }
        else
        {
            obj = nullptr;
        }
    }
    else
    {
        obj = nullptr;
    }

    return const_cast<Object*>(obj);
}

inline void Debug_Fail(const String* message, const String* file, Integer line, Integer column, const String* lineText)
{
    std::string msgText;

    if (message)
        msgText = std::format("assertion failed: {}", message->GetView());
    else
        msgText = "assertion failed.";

    Throw(SourceLocation(line, column, shared_string(file->GetView()), shared_string(lineText->GetView())), "{}", msgText);
}

} // fraze
