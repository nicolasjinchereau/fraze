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

inline Integer Object_GetHashCode(const Object& value)
{
    union {
        const Object* object;
        Integer integer;
    } u;

    u.object = &value;
    return u.integer;
}

} // fraze
