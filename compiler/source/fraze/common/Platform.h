/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once

#define FRAZE_INLINE __forceinline
#define RESTRICT __restrict

#define CONCAT_IMPL(x, y) x##y
#define CONCAT(x, y) CONCAT_IMPL(x, y)

#ifndef FRAZE_CODE_PROFILING
#  define FRAZE_CODE_PROFILING 0
#endif

#ifndef FRAZE_HEAP_DEBUG
#  define FRAZE_HEAP_DEBUG 0
#endif

#ifndef FRAZE_PRINT_EXECUTED_CODE
#  define FRAZE_PRINT_EXECUTED_CODE 0
#endif

#define FRAZE_OPTIMIZATION_OFF __pragma(optimize("", off))
#define FRAZE_OPTIMIZATION_ON __pragma(optimize("", on))