/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/common/SharedString.h>
#include <fraze/common/SourceLocation.h>

namespace fraze {

// source location of a runtime check (assert, null, bounds, type), referenced by site id
struct CheckSite
{
    shared_string message; // empty for asserts, which pass their message at runtime
    SourceLocation loc;
};

} // fraze
