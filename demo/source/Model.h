/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/common/Object.h>
#include <Types.h>
#include <string>

namespace fraze {

class Buffer;
class Graphics;
class Program;

class ModelImporter
{
    friend Graphics;
public:
    static Class* ImportModel(Program* program, Graphics* graphics, const std::string& path);
    static Class* CreateSphereMesh(Program* program, Graphics* graphics, Number radius, Integer segments, Integer rings, bool invert);
    static void ImportModelAsync(Program* program, Class& task, Graphics* graphics, const std::string& path);
};

} // fraze
