/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once

#ifndef NOMINMAX
#  define NOMINMAX
#endif
#include <Windows.h>
#include <windowsx.h>
#include <wrl/client.h>
#include <shellscalingapi.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <DirectXMath.h>
#undef FALSE
#undef TRUE
#undef FLOAT
#undef GetObject

template<class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
