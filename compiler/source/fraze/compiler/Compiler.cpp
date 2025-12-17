/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/ast/AST.h>
#include <fraze/ast/ASTPrinter.h>
#include <fraze/ast/CodePrinter.h>
#include <fraze/compiler/Compiler.h>
#include <fraze/compiler/CodeGenerator.h>
#include <fraze/compiler/SemanticAnalyzer.h>
#include <fraze/compiler/Lexer.h>
#include <fraze/compiler/Parser.h>
#include <fraze/compiler/NativeFunctions.h>
#include <fraze/program/TypeInfo.h>
#include <filesystem>
#include <system_error>

namespace fraze {

thread_local Compiler* Compiler::activeCompiler = nullptr;

Compiler::Compiler(std::string_view assetsPath)
{
    AddDirectory(assetsPath);
    AddFunction("WaitAsync", &WaitAsync);
    AddFunction("YieldAsync", &YieldAsync);
    AddFunction("Console.Write", &Console_Write);
    AddFunction("Console.WriteLine", &Console_WriteLine);
    AddFunction("Boolean.GetHashCode", &Boolean_GetHashCode);
    AddFunction("Integer.GetHashCode", &Integer_GetHashCode);
    AddFunction("Number.GetHashCode", &Number_GetHashCode);
    AddFunction("Object.GetHashCode", &Object_GetHashCode);
    AddFunction("String.GetHashCode", &String_GetHashCode);
    AddFunction("String.Split", &String_Split);
    AddFunction("GC.Collect", &GC_Collect);
    AddFunction("GC.Report", &GC_Report);
    AddFunction("Math.Fmod", &Math_Fmod);
    AddFunction("Math.Abs", &Math_Abs);
    AddFunction("Math.Sqrt", &Math_Sqrt);
    AddFunction("Math.Sin", &Math_Sin);
    AddFunction("Math.Cos", &Math_Cos);
    AddFunction("Math.Tan", &Math_Tan);
    AddFunction("Math.Asin", &Math_Asin);
    AddFunction("Math.Acos", &Math_Acos);
    AddFunction("Math.Atan", &Math_Atan);
    AddFunction("Math.Atan2", &Math_Atan2);
}

Compiler& Compiler::AddFile(std::string_view fileName)
{
    std::filesystem::path filePath(fileName);
    filePath.make_preferred();
    sourceFiles.try_emplace(filePath.string());
    return *this;
}

Compiler& Compiler::AddDirectory(std::string_view path)
{
    namespace fs = std::filesystem;

    const fs::path root(path);
    std::error_code ec;

    fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;

    for( ; it != end; it.increment(ec))
    {
        if(ec) {
            ec.clear();
            continue;
        }

        const fs::directory_entry& de = *it;

        if(!de.is_regular_file(ec))
        {
            if(ec)
                ec.clear();

            continue;
        }

        if(de.path().extension() == ".fz")
        {
            AddFile(de.path().string());
        }
    }

    return *this;
}

Compiler& Compiler::DisableAssert()
{
    assertEnabled = false;
    return *this;
}

Compiler& Compiler::DisableNullCheck()
{
    nullCheckEnabled = false;
    return *this;
}

Compiler& Compiler::DisableBoundsCheck()
{
    boundsCheckEnabled = false;
    return *this;
}

Compiler& Compiler::DisableTypeCheck()
{
    typeCheckEnabled = false;
    return *this;
}

Compiler& Compiler::PrintParsedCode()
{
    printParsedCode = true;
    return *this;
}

Compiler& Compiler::ExportAST(std::string_view outputPath)
{
    exportAST = true;
    astOutputPath = outputPath;
    return *this;
}

Compiler& Compiler::ExportBytecode(std::string_view outputPath)
{
    exportBytecode = true;
    bytecodeOutputPath = outputPath;
    return *this;
}

sptr<Program> Compiler::Compile()
{
    sptr<Program> program;

    try
    {
        ENFORCE(activeCompiler == nullptr, SourceLocation(), "There's already an active compiler on this thread");
        activeCompiler = this;

        sptr<ASTRoot> root = spnew<ASTRoot>();

        for(auto& [path, info] : sourceFiles)
        {
            Lexer lexer(path);
            info.tokens = lexer.Tokenize();

            Parser parser(info.tokens);
            parser.Parse(root);
        }

        if(exportAST)
        {
            std::filesystem::path outputPath = !astOutputPath.empty() ? astOutputPath : ".";
            std::filesystem::path outputFile = outputPath;
            outputFile /= "AST.txt";
            outputFile.make_preferred();

            std::ofstream stream(outputFile);
            fraze::ASTPrinter printer(stream, 2);
            printer.VisitChild(root);
        }

        SemanticAnalyzer analyzer;
        analyzer.VisitChild(root);

        if(printParsedCode)
        {
            std::stringstream stream;

            fraze::CodePrinter printer(stream, 4);
            printer.VisitChild(root);

            std::cout << stream.str() << std::endl << std::endl;
        }

        CodeGenerator generator;
        generator.VisitChild(root);

        program = std::move(generator.program);

        activeCompiler = nullptr;

        if(exportBytecode)
        {
            std::unordered_map<std::string_view, std::vector<FunctionInfo*>> functions;
            std::vector<std::string_view> files;

            for(sptr<TypeInfo>& ty : program->typeInfo)
            {
                if(FunctionInfo* funcInfo = ty->ToFunctionInfo())
                {
                    functions[funcInfo->loc.file].push_back(funcInfo);
                }
            }

            for(auto& [file, funcInfoList] : functions)
            {
                std::ranges::sort(funcInfoList, [](FunctionInfo* a, FunctionInfo* b) {
                    return a->loc.line < b->loc.line;
                });
            }

            using std::ranges::find_if;

            std::filesystem::path outputPath = !bytecodeOutputPath.empty() ? bytecodeOutputPath : ".";

            std::error_code ec;
            std::filesystem::create_directories(outputPath, ec);
            
            if(ec)
            {
                Throw("Failed to create output path for bytecode: {}", ec.message());
            }

            for(auto& [path, info] : sourceFiles)
            {
                std::filesystem::path outputFile = outputPath;
                outputFile /= utility::ReplaceAllOf(path, "\\/", '.') + ".csv";
                outputFile.make_preferred();

                std::ifstream fin(path);
                std::ofstream fout(outputFile);

                size_t lineNum = 1;
                std::string line;
                std::stringstream temp;

                std::vector<FunctionInfo*>& funcList = functions[path];

                FunctionInfo* currentFunction = nullptr;
                size_t opCodeIndex = 0;

                // print all function code
                for( ; std::getline(fin, line); ++lineNum)
                {
                    // code-text column
                    std::string lineStr = utility::ReplaceAll(line, "\"", "\"\"");
                    fout << "\"" << lineStr << "\"" << ",";

                    // op-code column
                    fout << "\"";

                    if(!funcList.empty() && lineNum >= funcList.front()->loc.line)
                    {
                        currentFunction = funcList.front();
                        funcList.erase(funcList.begin());
                        opCodeIndex = currentFunction->codeStart;
                    }

                    if(currentFunction)
                    {
                        for(size_t opCount = 0;
                            opCodeIndex < currentFunction->codeEnd && program->locations[opCodeIndex].line <= lineNum;
                            ++opCodeIndex)
                        {
                            if(opCount++ > 0)
                                fout << "\n";

                            temp.str("");
                            program->PrintOperation(opCodeIndex, temp);
                            fout << utility::ReplaceAll(temp.str(), "\"", "\"\"");
                        }

                        // If we're about to increment to the next function, print the remaining operations
                        if(!funcList.empty() && (lineNum + 1) >= funcList.front()->loc.line)
                        {
                            for(size_t opCount = 0; opCodeIndex < currentFunction->codeEnd; ++opCodeIndex)
                            {
                                if(opCount++ > 0)
                                    fout << "\n";

                                temp.str("");
                                program->PrintOperation(opCodeIndex, temp);
                                fout <<  utility::ReplaceAll(temp.str(), "\"", "\"\"");
                            }
                        }

                        if(opCodeIndex == currentFunction->codeEnd)
                            currentFunction = nullptr;
                    }

                    fout << "\"" << "\n";
                }

            }
            
            // print all section code
            {
                std::filesystem::path outputFile = outputPath;
                outputFile /= "all-sections.csv";
                outputFile.make_preferred();

                std::ofstream fout(outputFile);
                std::stringstream temp;

                for(auto& ti : program->typeInfo)
                {
                    if(auto sect = ti->ToSectionInfo())
                    {
                        auto sectName = !sect->qualifiedName.empty() ? sect->qualifiedName : "global";
                        fout << "\"" << "section " << sectName << "\"" << ",";

                        fout << "\"";

                        size_t opCount = 0;

                        auto it = program->code.begin() + sect->codeStart;
                        auto end = program->code.begin() + sect->codeEnd;
                        while(it != end)
                        {
                            if(opCount++ > 0)
                                fout << "\n";

                            temp.str("");
                            size_t i = ( it - program->code.begin() );
                            program->PrintOperation(i, temp);

                            std::string opStr = utility::ReplaceAll(temp.str(), "\"", "\"\"");
                            fout << opStr;

                            ++it;
                        }

                        fout << "\"" << "\n";
                    }
                }
            }
        }
    }
    catch(...)
    {
        activeCompiler = nullptr;
        throw;
    }

    return program;
}

Compiler* Compiler::GetActiveCompiler()
{
    return activeCompiler;
}

sptr<IExternalFunction> Compiler::GetFunction(std::string_view qualifiedName, std::string_view signature)
{
    sptr<IExternalFunction> ret;

    auto it = functions.find(qualifiedName);
    if(it != functions.end())
        ret = it->second;

    return ret;
}

IntrinsicFunction Compiler::GetIntrinsic(std::string_view qualifiedName, std::string_view signature)
{
    IntrinsicFunction ret = nullptr;

    auto it = intrinsics.find(qualifiedName);
    if(it != intrinsics.end())
        ret = it->second;

    std::string nameAndSignature;
    nameAndSignature.reserve(qualifiedName.length() + 1 + signature.length());
    nameAndSignature += qualifiedName;
    nameAndSignature += ":";
    nameAndSignature += signature;

    it = intrinsics.find(nameAndSignature);
    if(it != intrinsics.end())
        ret = it->second;

    return ret;
}

} // fraze
