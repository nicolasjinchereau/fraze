/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/common/Object.h>
#include <fraze/program/OpCode.h>

namespace fraze {

struct Operation
{
    OpCode code;

    union {
        uint64_t arg1_u64;
        struct {
            uint32_t arg1_u32a;
            uint32_t arg1_u32b;
        };
        Integer arg1_i64;
        Number arg1_f64;
        const char* arg1_cstr;
    };

    union {
        uint64_t arg2_u64;
        struct {
            uint32_t arg2_u32a;
            uint32_t arg2_u32b;
        };
        Integer arg2_i64;
        Number arg2_f64;
        const char* arg2_cstr;
    };

    Operation(OpCode op, auto arg1, auto arg2)
        : code(op)
    {
        if constexpr(std::is_floating_point_v<decltype(arg1)>)
            arg1_f64 = arg1;
        else if constexpr(std::is_unsigned_v<decltype(arg1)>)
            arg1_u64 = arg1;
        else if constexpr(std::is_signed_v<decltype(arg1)>)
            arg1_i64 = arg1;
        else if constexpr(is_c_string_v<decltype(arg1)>)
            arg1_cstr = arg1;
        else
            static_assert(0);

        if constexpr(std::is_floating_point_v<decltype(arg2)>)
            arg2_f64 = arg2;
        else if constexpr(std::is_unsigned_v<decltype(arg2)>)
            arg2_u64 = arg2;
        else if constexpr(std::is_signed_v<decltype(arg2)>)
            arg2_i64 = arg2;
        else if constexpr(is_c_string_v<decltype(arg2)>)
            arg2_cstr = arg2;
        else
            static_assert(0);
    }

    Operation(OpCode op, auto arg1)
        : Operation(op, arg1, 0ull)
    {
    }

    Operation(OpCode op)
        : Operation(op, 0ull, 0ull)
    {
    }
};

} // fraze
