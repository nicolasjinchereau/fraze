/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/common/Object.h>

namespace fraze {

class Program;

class File
{
public:
    static String* ReadAllText(Program* program, const String& path);
    static void ReadAllTextAsync(Program* program, Class& task, const String& path);
};

} // fraze
