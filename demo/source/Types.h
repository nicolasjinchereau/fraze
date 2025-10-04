/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/common/Object.h>
#include <cassert>
#include <cstdint>
#include <array>

namespace fraze {

enum class Keycode : Integer
{
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4,
    Num5, Num6, Num7, Num8, Num9,
    F1, F2, F3, F4, F5, F6,
    F7, F8, F9, F10, F11, F12,
    Back,
    Forward,
    VolumeUp,
    VolumeDown,
    VolumeMute,
    Space,
    Backspace,
    Del,
    LeftArrow,
    RightArrow,
    UpArrow,
    DownArrow,
    Alt,
    Ctrl,
    Shift,
    Enter,
    Escape,
    Equals,
    Minus,
    LeftBracket,
    RightBracket,
    Quote,
    Semicolon,
    Backslash,
    Comma,
    Slash,
    Period,
    Grave,
    Tab,

    // OS specific keycodes not included above will be returned from TranslateKey as (Keycode::Unknown + keycode)
    Unknown
};

enum class DrawMode : Integer
{
    Points,
    Lines,
    LineStrip,
    Triangles,
    TriangleStrip
};

enum class CullMode : Integer
{
    Off,
    Back,
    Front,
};

enum class DepthTest : Integer
{
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
};

enum class BlendFactor : Integer
{
    Zero,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    SrcAlphaSaturate,
    ConstColor,
    OneMinusConstColor,
    Src1Color,
    OneMinusSrc1Color,
    Src1Alpha,
    OneMinusSrc1Alpha
};

enum class BlendOperation : Integer
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

enum class BufferType : Integer
{
	Vertex,
	Index
};

enum class BufferUsage : Integer
{
	Default,
	Stream,
	Static,
	Dynamic
};

enum class BufferCPUAccess : Integer
{
	None,
	ReadOnly,
	WriteOnly,
	ReadWrite
};

enum class BufferMapAccess : Integer
{
	ReadOnly,
	WriteOnly,
	ReadWrite,
	WriteDiscard,
	WriteNoSync,
};

enum class PixelDataFormat : Integer
{
    Unspecified,
    Alpha8,
    RGB24,
    RGBA32,
    RGBAFloat
};

enum class TextureWrapMode : Integer
{
    Clamp,
    Repeat,
};

enum class TextureFilterMode : Integer
{
    Point,
    Bilinear,
    Trilinear,
};

struct IVec2
{
    Integer x = 0;
    Integer y = 0;
};

struct Vec2
{
    Number x = 0;
    Number y = 0;
};

struct Vec3
{
    Number x = 0;
    Number y = 0;
    Number z = 0;
};

struct Vec4
{
    Number x = 0;
    Number y = 0;
    Number z = 0;
    Number w = 0;
};

struct IVec4
{
    Integer x = 0;
    Integer y = 0;
    Integer z = 0;
    Integer w = 0;
};

struct Quat
{
    Vec3 v;
    Number w = 0;
};

struct Mat4
{
    Number m11, m12, m13, m14;
    Number m21, m22, m23, m24;
    Number m31, m32, m33, m34;
    Number m41, m42, m43, m44;
};

struct Vertex
{
    Vec3 pos;
    Vec2 tex;
    Vec3 norm;
};

struct Color
{
	Number r = 0;
	Number g = 0;
	Number b = 0;
	Number a = 0;
};

struct IntRect
{
	Integer x = 0;
	Integer y = 0;
	Integer w = 0;
	Integer h = 0;
};

struct Bone
{
    String* linkNodeName{};
    Mat4 meshBindMatrix;
    Mat4 invBoneBindMatrix;
};

struct Transform
{
    Vec3 pos;
    Quat rot;
    Vec3 scale;
};

struct Keyframe
{
    Number time;
    Transform value;
};

struct AnimationTrack
{
    String* nodeName{};
    Array<Keyframe>* frames{};
};

struct AnimationClip
{
    String* name{};
    Number length{};
    Array<AnimationTrack>* tracks{};
};

template<class T>
inline auto AsFloatArray(const T& value)
{
    static_assert(sizeof(T) % sizeof(Number) == 0,
        "size of type must be a multiple of 'Number' size");
    
    constexpr size_t count = sizeof(T) / sizeof(Number);
    constexpr size_t destSize = count * sizeof(float);
    const Number* src = reinterpret_cast<const Number*>(&value);
    
    std::array<float, count> ret;
    
    for(size_t i = 0; i != count; ++i)
        ret[i] = static_cast<float>(src[i]);

    return ret;
}

inline void ToFloatBuffer(const Number* src, size_t srcCount, float* dest, size_t destSize)
{
    size_t requiredDestSize = srcCount * sizeof(Number) / 2;
    assert(destSize >= requiredDestSize);

    for(size_t i = 0; i != srcCount; ++i)
        dest[i] = static_cast<float>(src[i]);

}

} // fraze
