/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <cassert>
#include <cstddef>
#include <iomanip>
#include <ranges>
#include <span>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_set>
#include <fraze/ast/ASTFwd.h>
#include <fraze/common/Object.h>
#include <fraze/common/Pointers.h>
#include <fraze/common/ExternalFunction.h>
#include <fraze/common/Extensions.h>
#include <fraze/compiler/Lexer.h>
#include <fraze/program/Program.h>
#include <fraze/memory/ScopedAllocator.h>

namespace fraze {

struct SourceFile
{
    std::vector<Token> tokens;
};

class Parser;
class SemanticAnalyzer;
class VMCodeGenerator;
class JITCodeGenerator;
class Scope;
class Type;

enum class CodeGenerator
{
    VM,
    JIT,
};

// applies to JIT/MIR only
enum class Optimization : uint32_t
{
    Off     = 0, // fast generation
    Minimal = 1, // register allocation and combiner
    Default = 2, // adds GVN and CCP
    Maximum = 3, // everything
};

class Compiler
{
    bool exportAST = false;
    bool exportLoweredCode = false;
    bool exportBytecode = false;
    bool assertEnabled = true;
    bool nullCheckEnabled = true;
    bool boundsCheckEnabled = true;
    bool typeCheckEnabled = true;
    CodeGenerator codeGenerator = CodeGenerator::VM;
    Optimization optimization = Optimization::Default;
    std::string astOutputPath;
    std::string loweredCodeOutputPath;
    std::string bytecodeOutputPath;

    string_view_map<SourceFile> sourceFiles;
    string_view_map<sptr<IExternalFunction>> functions;
    std::vector<sptr<Type>> types;
    
    static thread_local Compiler* activeCompiler;

    friend Parser;
    friend SemanticAnalyzer;
    friend VMCodeGenerator;
    friend JITCodeGenerator;
    friend Type;

public:

    Compiler& AddFrazeRuntime(std::string_view assetsPath);
    Compiler& AddFile(std::string_view file);
    Compiler& AddDirectory(std::string_view path);
    Compiler& DisableAssert();
    Compiler& DisableNullCheck();
    Compiler& DisableBoundsCheck();
    Compiler& DisableTypeCheck();
    Compiler& ExportAST(std::string_view outputPath = "");
    Compiler& ExportLoweredCode(std::string_view outputPath = "");
    Compiler& ExportBytecode(std::string_view outputPath = "");
    Compiler& SetCodeGenerator(CodeGenerator codeGenerator);
    Compiler& SetOptimization(Optimization optimization);

    template<auto Func>
    Compiler& AddFunction(const std::string& qualifiedName) {
        functions[qualifiedName] = spnew<ExternalFunction<Func>>(qualifiedName);
        return *this;
    }

    template<auto Func>
    Compiler& AddFunction(const std::string& qualifiedName, const std::string& signature) {
        AddFunction<Func>(std::format("{}:{}", qualifiedName, signature));
        return *this;
    }

    sptr<Program> Compile();
    static Compiler* GetActiveCompiler();

    sptr<IExternalFunction> GetFunction(std::string_view qualifiedName, std::string_view signature);
    bool IsAssertEnabled() const { return assertEnabled; }
    bool IsNullCheckEnabled() const { return nullCheckEnabled; }
    bool IsBoundsCheckEnabled() const { return boundsCheckEnabled; }
    bool IsTypeCheckEnabled() const { return typeCheckEnabled; }
};

} // fraze
