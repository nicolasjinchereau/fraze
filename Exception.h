/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2022 Nicolas Jinchereau. All rights reserved.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include <exception>
#include <string>
#include <source_location>
#include <sstream>
#include <utf8.h>

class Exception : public std::exception
{
    std::source_location loc;
    std::string msg;
public:

    Exception(const std::string& msg, std::source_location loc = std::source_location::current())
        : loc(loc), msg(msg)
    {
    }

    Exception(const std::u8string& msg, std::source_location loc = std::source_location::current())
        : loc(loc), msg((const char*)msg.data(), (const char*)msg.data() + msg.size())
    {
    }

    Exception(std::source_location loc = std::source_location::current())
        : loc(loc), msg()
    {
    }

    const std::string& GetMessage() const {
        return msg;
    }

    const std::source_location& GetLocation() const {
        return loc;
    }

    void Print()
    {
        printf("Exception has occured:\n file: %s\n line: %d\n function: %s\n message: %s\n",
            loc.file_name(), (int)loc.line(), loc.function_name(), msg.c_str());
    }

    virtual char const* what() const override {
        return msg.c_str();
    }
};
