require "android-lib"

workspace "zlib"
    configurations { "Debug", "Release" }
    --architecture "x86_64"
    location ("projects/" .. os.target())

project "zlib"
    kind "StaticLib"
    language "C"
    --architecture "x86_64"

    location ("projects/" .. os.target())
    targetname "zlib"

    files {
        "source/adler32.c",
        "source/compress.c",
        "source/crc32.c",
        "source/crc32.h",
        "source/deflate.c",
        "source/deflate.h",
        "source/gzclose.c",
        "source/gzguts.h",
        "source/gzlib.c",
        "source/gzread.c",
        "source/gzwrite.c",
        "source/infback.c",
        "source/inffast.c",
        "source/inffast.h",
        "source/inffixed.h",
        "source/inflate.c",
        "source/inflate.h",
        "source/inftrees.c",
        "source/inftrees.h",
        "source/trees.c",
        "source/trees.h",
        "source/uncompr.c",
        "source/zconf.h",
        "source/zlib.h",
        "source/zutil.c",
        "source/zutil.h"
    }

    sysincludedirs {
        "source/"
    }

    androidabis {
		"armeabi-v7a", "arm64-v8a"
    }

    androidminsdkversion "android-16"

    filter { "action:vs*" }
        objdir ("!obj/$(PlatformName)/$(Configuration)")
        targetdir ("lib/$(PlatformName)/$(Configuration)")
        characterset "MBCS"

    filter { "action:xcode*" }
        objdir ("!obj/$(PLATFORM_NAME)/$(CONFIGURATION)")
        targetdir ("lib/$(PLATFORM_NAME)/$(CONFIGURATION)")

    filter { "action:android*" }
        objdir ("obj/android/$(TARGET_ARCH_ABI)/$(APP_OPTIM)")
        targetdir ("lib/android/$(TARGET_ARCH_ABI)/$(APP_OPTIM)")

    filter "configurations:Debug"
        defines {
            "DEBUG",
            "ZLIB_DEBUG",
            "_CRT_SECURE_NO_DEPRECATE",
            "_CRT_SECURE_NO_WARNINGS"
        }
        symbols "On"

    filter "configurations:Release"
        defines {
            "NDEBUG",
            "_CRT_SECURE_NO_DEPRECATE",
            "_CRT_SECURE_NO_WARNINGS"
        }
        optimize "Speed"
        inlining "Auto"
        floatingpoint "Fast"
        vectorextensions "SSE2"

    filter { "action:xcode*" }
        xcodebuildsettings {           
            ["CLANG_CXX_LANGUAGE_STANDARD"] = "gnu++20";
            ["GCC_C_LANGUAGE_STANDARD"] = "gnu11";
            ["CLANG_CXX_LIBRARY"] = "libc++";
        }
    
    filter { "action:vs*" }
        cppdialect "C++20"
        cdialect "C11"

