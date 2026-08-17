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
class CodeGenerator;
class Scope;
class Type;

class Compiler
{
    bool printParsedCode = false;
    bool exportAST = false;
    bool exportBytecode = false;
    bool assertEnabled = true;
    bool nullCheckEnabled = true;
    bool boundsCheckEnabled = true;
    bool typeCheckEnabled = true;
    std::string astOutputPath;
    std::string bytecodeOutputPath;

    string_view_map<SourceFile> sourceFiles;
    string_view_map<sptr<IExternalFunction>> functions;
    string_view_map<IntrinsicFunction> intrinsics;
    std::vector<sptr<Type>> types;
    
    static thread_local Compiler* activeCompiler;

    friend Parser;
    friend SemanticAnalyzer;
    friend CodeGenerator;
    friend Type;

public:

    Compiler(std::string_view assetsPath);

    Compiler& AddFile(std::string_view file);
    Compiler& AddDirectory(std::string_view path);
    Compiler& DisableAssert();
    Compiler& DisableNullCheck();
    Compiler& DisableBoundsCheck();
    Compiler& DisableTypeCheck();
    Compiler& PrintParsedCode();
    Compiler& ExportAST(std::string_view outputPath = "");
    Compiler& ExportBytecode(std::string_view outputPath = "");

    template<class Ret, class... Args>
    Compiler& AddFunction(const std::string& qualifiedName, Ret(*func)(Args...)) {
        functions[qualifiedName] = std::make_shared<ExternalFunction<Ret, Args...>>(qualifiedName, func);
        return *this;
    }

    template<class Ret, class... Args>
    Compiler& AddFunction(const std::string& qualifiedName, const std::string& signature, Ret(*func)(Args...)) {
        AddFunction(std::format("{}:{}", qualifiedName, signature), func);
        return *this;
    }

    Compiler& AddIntrinsic(const std::string& qualifiedName, IntrinsicFunction intrinsic) {
        intrinsics[qualifiedName] = intrinsic;
        return *this;
    }

    Compiler& AddIntrinsic(const std::string& qualifiedName, const std::string& signature, IntrinsicFunction intrinsic) {
        AddIntrinsic(std::format("{}:{}", qualifiedName, signature), intrinsic);
        return *this;
    }

    sptr<Program> Compile();
    static Compiler* GetActiveCompiler();

    sptr<IExternalFunction> GetFunction(std::string_view qualifiedName, std::string_view signature);
    IntrinsicFunction GetIntrinsic(std::string_view qualifiedName, std::string_view signature);
    bool IsAssertEnabled() const { return assertEnabled; }
    bool IsNullCheckEnabled() const { return nullCheckEnabled; }
    bool IsBoundsCheckEnabled() const { return boundsCheckEnabled; }
    bool IsTypeCheckEnabled() const { return typeCheckEnabled; }
};

} // fraze
