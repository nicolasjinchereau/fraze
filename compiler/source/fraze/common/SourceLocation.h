/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <source_location>
#include <fraze/common/SharedString.h>

namespace fraze {

struct SourceLocation
{
    shared_string file;
    uint32_t line;
    uint32_t column;
    uint32_t lineStart; // byte offset in 'file' of the line's first character
    uint32_t lineEnd;   // byte offset in 'file' of the line's terminating '\n' (or end of file)

    SourceLocation(shared_string file = shared_string(), uint32_t line = 0, uint32_t column = 0, uint32_t lineStart = 0, uint32_t lineEnd = 0)
        : file(file),
          line(line),
          column(column),
          lineStart(lineStart),
          lineEnd(lineEnd)
    {
    }

    SourceLocation(const std::source_location& loc)
        : file(loc.file_name()),
          line(loc.line()),
          column(loc.column()),
          lineStart(0),
          lineEnd(0)
    {
    }

    // reads the line from 'file'; only meant for error reporting, so nothing is cached
    std::string GetLineText() const;

    bool operator==(const SourceLocation& other) const
    {
        return file == other.file && 
            line == other.line &&
            column == other.column &&
            lineStart == other.lineStart &&
            lineEnd == other.lineEnd;
    }
};

} // fraze
