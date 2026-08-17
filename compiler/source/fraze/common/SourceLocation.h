/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cstddef>
#include <string>
#include <source_location>
#include <fraze/common/SharedString.h>

namespace fraze {

struct SourceLocation
{
    size_t line;
    size_t column;
    shared_string file;
    shared_string lineText;

    SourceLocation(size_t line = 0, size_t column = 0, shared_string file = shared_string(), shared_string lineText = shared_string())
        : line(line),
          column(column),
          file(file),
          lineText(lineText)
    {
    }

    SourceLocation(const std::source_location& loc)
        : line(loc.line()),
          column(loc.column()),
          file(loc.file_name()),
          lineText(loc.function_name())
    {
    }

    bool operator==(const SourceLocation& other) const
    {
        return line == other.line &&
            column == other.column &&
            file == other.file &&
            lineText == other.lineText;
    }
};

} // fraze
