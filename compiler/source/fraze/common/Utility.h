/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <fraze/common/Exception.h>

using namespace std::string_literals;

namespace fraze {
namespace utility {

template<class T>
inline size_t IndexOf(const std::vector<T>& cont, const T& value)
{
    size_t i = 0;
    size_t sz = cont.size();

    for( ; i != sz; ++i)
    {
        if(cont[i] == value)
            break;
    }

    return i;
}

inline std::vector<std::string> Split(std::string_view str, std::string_view delim)
{
    std::vector<std::string> ret;

    size_t a = 0;
    size_t b = str.find(delim, a);

    while(b != std::string::npos)
    {
        if(b - a != 0)
            ret.push_back(std::string(str.begin() + a, str.begin() + b));

        a = b + delim.size();
        b = str.find(delim, a);
    }

    if(a != str.size())
        ret.push_back(std::string(str.begin() + a, str.end()));

    return ret;
}

inline std::string Join(const std::vector<std::string>& parts, std::string_view sep)
{
    std::string ret;

    if(!parts.empty())
    {
        size_t sz = 0;

        for(auto& p : parts)
            sz += p.length();

        sz += (parts.size() - 1) * sep.length();

        ret.reserve(sz);

        for(auto& p : parts)
        {
            if(!ret.empty())
                ret.append(sep);

            ret.append(p);
        }
    }

    return ret;
}

inline std::string ReplaceAll(std::string_view in, std::string_view what, std::string_view with)
{
    std::string result;
    result.reserve(in.length());

    std::string::size_type first = 0;
    
    for (std::string::size_type last = 0;
        (last = in.find(what.data(), first, what.length())) != std::string::npos;
        first = last + what.length())
    {
        result.append(in.data() + first, in.data() + last);
        result.append(with);
    }

    if (first != in.length())
        result.append(in.data() + first, in.data() + in.length());

    return result;
}

inline std::string ReplaceAllOf(std::string_view in, std::string_view charsToReplace, char replacement)
{
    std::string result;
    result.reserve(in.size());

    for(char ch : in)
    {
        if(charsToReplace.find(ch) != std::string::npos)
            result += replacement;
        else
            result += ch;
    }

    return result;
}

inline std::string ReadFile(const std::filesystem::path& path)
{
    std::ifstream fin(path, std::ios::in | std::ios::binary);

    if (!fin.good())
        throw Exception("failed to open file: "s + path.string());

    fin.seekg(0, std::ios::end);
    auto sz = (size_t)fin.tellg();
    fin.seekg(0, std::ios::beg);

    std::string ret;

    if(sz != 0)
    {
        ret.resize(sz);
        if(!fin.read(reinterpret_cast<char*>(ret.data()), sz))
            throw Exception("file read failed: "s + path.string());
    }

    return ret;
}

template<class T>
constexpr const T& FirstTruthy(const T& value)
{
    return value;
}

template<class T, class... Rest>
constexpr const T& FirstTruthy(const T& first, const Rest&... rest)
{
    return first ? first : FirstTruthy(rest...);
}

class TypeNameSplitter
{
    std::string_view typeName;
public:

    TypeNameSplitter(std::string_view typeName)
        : typeName(typeName)
    {
    }

    class iterator
    {
        std::string_view typeName;
        size_t first = 0;
        size_t last = 0;
        int depth = 0;

        friend TypeNameSplitter;
    public:

        iterator(std::string_view typeName)
            : typeName(typeName)
        {
            while(last < typeName.size())
            {
                char c = typeName[last];

                if(c == '<')
                    ++depth;
                else if(c == '>')
                    --depth;
                else if(c == '.' && depth == 0)
                    break;

                ++last;
            }
        }

        iterator(std::string_view typeName, size_t first, size_t last)
            : typeName(typeName), first(first), last(last)
        {
        }

        std::string_view operator*() const {
            return typeName.substr(first, last - first);
        }

        iterator& operator++()
        {
            if(last == typeName.size())
            {
                first = last;
                return *this;
            }

            ++last;
            first = last;

            while(last < typeName.size())
            {
                char c = typeName[last];

                if(c == '<')
                    ++depth;
                else if(c == '>')
                    --depth;
                else if(c == '.' && depth == 0)
                    break;

                ++last;
            }

            return *this;
        }

        bool operator!=(const iterator& other) const {
            return first != other.first;
        }
    };

    iterator begin() const {
        return iterator(typeName);
    }

    iterator end() const {
        return iterator(typeName, typeName.size(), typeName.size());
    }
};

} // utility
} // fraze
