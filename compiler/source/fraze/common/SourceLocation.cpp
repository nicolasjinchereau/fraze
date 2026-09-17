/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/common/SourceLocation.h>
#include <fstream>

namespace fraze {

std::string SourceLocation::GetLineText() const
{
    if(lineEnd <= lineStart)
        return {};

    std::ifstream stream(std::string(file.view()), std::ios::in | std::ios::binary);
    std::string text(lineEnd - lineStart, '\0');

    if(!stream.seekg(lineStart) || !stream.read(text.data(), text.size()))
        return {};

    // the lexer ends lines at '\n', so CRLF files leave a '\r' behind
    if(text.back() == '\r')
        text.pop_back();

    return text;
}

} // fraze
