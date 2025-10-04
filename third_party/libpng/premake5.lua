require "android-lib"

workspace "libpng"
    configurations { "Debug", "Release" }
    location ("projects/" .. os.target())

project "libpng"
    kind "StaticLib"
    language "C"
    --architecture "x86_64"

    location ("projects/" .. os.target())
    targetname "png"
    
    files {
        "source/png.c",
        "source/pngerror.c",
        "source/pngget.c",
        "source/pngmem.c",
        "source/pngpread.c",
        "source/pngread.c",
        "source/pngrio.c",
        "source/pngrtran.c",
        "source/pngrutil.c",
        "source/pngset.c",
        "source/pngtrans.c",
        "source/pngwio.c",
        "source/pngwrite.c",
        "source/pngwtran.c",
        "source/pngwutil.c",
    }

    sysincludedirs {
        "source/",
        "../zlib/source/"
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
            "_CRT_SECURE_NO_WARNINGS",
            "_CRT_NONSTDC_NO_DEPRECATE"
        }
        symbols "On"

    filter "configurations:Release"
        defines {
            "NDEBUG",
            "_CRT_SECURE_NO_WARNINGS",
            "_CRT_NONSTDC_NO_DEPRECATE"
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