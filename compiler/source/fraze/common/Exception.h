/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <exception>
#include <string>
#include <sstream>
#include <source_location>
#include <filesystem>
#include <fraze/common/SourceLocation.h>

namespace fraze {

#define BREAK_IF( predicate ) if( predicate ) __debugbreak();

class Exception : public std::exception
{
    std::string msg;
    std::string desc;
    SourceLocation loc;

public:
    Exception(
        const std::string& msg,
        const SourceLocation& loc = SourceLocation())
        : msg(msg), loc(loc)
    {
        CreateDescription();
    }

    Exception(
        std::string&& msg,
        const SourceLocation& loc = SourceLocation())
        : msg(std::move(msg)), loc(loc)
    {
        CreateDescription();
    }

    const std::string& GetMessage() const {
        return msg;
    }

    const std::string& GetDescription() const {
        return desc;
    }

    const SourceLocation& GetLocation() const {
        return loc;
    }

    virtual char const* what() const override {
        return desc.c_str();
    }

private:
    void CreateDescription()
    {
        if(loc.column != 0)
        {
            std::string hint;

            size_t ct = loc.column - 1;
            hint.reserve(ct + 1);
            hint.append(ct, '~');
            hint.append(1, '^');
            
            desc = std::format("{}({},{}): {}\n{}\n{}",
                loc.file, loc.line, loc.column, msg,
                loc.lineText,
                hint);
        }
        else
        {
            desc = msg;
        }
    }
};

extern bool ENFORCE_BreakOnError;

#ifdef _MSC_VER
#  define DEBUG_BREAK()           \
    do {                          \
        if(ENFORCE_BreakOnError)  \
            __debugbreak();       \
    } while(false)
#else
#  define DEBUG_BREAK()
#endif

#define ENFORCE(cond, loc, fmt, ...)                                        \
do {                                                                        \
    if(!(cond))                                                             \
    {                                                                       \
        DEBUG_BREAK();                                                      \
        throw Exception(std::format(fmt __VA_OPT__(,) __VA_ARGS__), (loc)); \
    }                                                                       \
} while(false)

#ifndef NDEBUG
#  define ENFORCE_DBG(...) ENFORCE(__VA_ARGS__)
#  define DEBUG_BREAK_DBG() DEBUG_BREAK()
#else
#  define ENFORCE_DBG(...) ((void)0)
#  define DEBUG_BREAK_DBG() ((void)0)
#endif

template<class... Args>
inline void Throw(
    const SourceLocation& loc,
    const std::format_string<Args...> fmt,
    Args&&... args)
{
    DEBUG_BREAK_DBG();
    throw Exception(std::format(fmt, std::forward<Args>(args)...), loc);
}

template<class... Args>
inline void Throw(
    const std::format_string<Args...> fmt,
    Args&&... args)
{
    DEBUG_BREAK_DBG();
    throw Exception(std::format(fmt, std::forward<Args>(args)...), SourceLocation());
}

} // fraze
