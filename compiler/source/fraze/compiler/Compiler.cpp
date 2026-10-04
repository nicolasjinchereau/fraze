/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/ast/AST.h>
#include <fraze/ast/ASTPrinter.h>
#include <fraze/ast/CodePrinter.h>
#include <fraze/compiler/Compiler.h>
#include <fraze/compiler/JITCodeGenerator.h>
#include <fraze/compiler/VMCodeGenerator.h>
#include <fraze/compiler/SemanticAnalyzer.h>
#include <fraze/compiler/Lexer.h>
#include <fraze/compiler/Parser.h>
#include <fraze/compiler/NativeFunctions.h>
#include <fraze/program/ProgramDiagnostics.h>
#include <fraze/program/TypeInfo.h>
#include <filesystem>
#include <system_error>

namespace fraze {

thread_local Compiler* Compiler::activeCompiler = nullptr;

Compiler& Compiler::AddFrazeRuntime(std::string_view assetsPath)
{
    AddDirectory(assetsPath);
    AddFunction<&WaitAsync>("WaitAsync");
    AddFunction<&YieldAsync>("YieldAsync");
    AddFunction<&Console_Write>("Console.Write");
    AddFunction<&Console_WriteLine>("Console.WriteLine");
    AddFunction<&Boolean_GetHashCode>("Boolean.GetHashCode");
    AddFunction<&Integer_GetHashCode>("Integer.GetHashCode");
    AddFunction<&Number_GetHashCode>("Number.GetHashCode");
    AddFunction<&Object_GetHashCode>("Object.GetHashCode");
    AddFunction<&String_GetHashCode>("String.GetHashCode");
    AddFunction<&String_Split>("String.Split");
    AddFunction<&String_Concat>("String.Concat");
    AddFunction<&String_Equals>("String.Equals");
    AddFunction<&String_FromBool>("String.FromBool");
    AddFunction<&String_FromInt>("String.FromInt");
    AddFunction<&String_FromNum>("String.FromNum");
    AddFunction<&String_FromEnum>("String.FromEnum");
    AddFunction<&GC_Collect>("GC.Collect");
    AddFunction<&GC_Report>("GC.Report");
    AddFunction<&Array_GetCount>("Array.GetCount");
    AddFunction<&Array_GetSize>("Array.GetSize");
    AddFunction<&Type_Find>("Type.Find");
    AddFunction<&Type_NewClass>("Type.NewClass");
    AddFunction<&Type_NewArray>("Type.NewArray");
    AddFunction<&Type_GetName>("Type.GetName");
    AddFunction<&Type_IsInstance>("Type.IsInstance");
    AddFunction<&Type_AsInstance>("Type.AsInstance");
    AddFunction<&Debug_Fail>("Debug.Fail");
    AddFunction<&Math_Fmod>("Math.Fmod");
    AddFunction<&Math_Abs>("Math.Abs");
    AddFunction<&Math_Sqrt>("Math.Sqrt");
    AddFunction<&Math_Sin>("Math.Sin");
    AddFunction<&Math_Cos>("Math.Cos");
    AddFunction<&Math_Tan>("Math.Tan");
    AddFunction<&Math_Asin>("Math.Asin");
    AddFunction<&Math_Acos>("Math.Acos");
    AddFunction<&Math_Atan>("Math.Atan");
    AddFunction<&Math_Atan2>("Math.Atan2");
    return *this;
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

Compiler& Compiler::ExportAST(std::string_view outputPath)
{
    exportAST = true;
    astOutputPath = outputPath;
    return *this;
}

Compiler& Compiler::ExportLoweredCode(std::string_view outputPath)
{
    exportLoweredCode = true;
    loweredCodeOutputPath = outputPath;
    return *this;
}

Compiler& Compiler::ExportBytecode(std::string_view outputPath)
{
    exportBytecode = true;
    bytecodeOutputPath = outputPath;
    return *this;
}

Compiler& Compiler::SetCodeGenerator(CodeGenerator codeGenerator)
{
    this->codeGenerator = codeGenerator;
    return *this;
}

Compiler& Compiler::SetOptimization(Optimization optimization)
{
    this->optimization = optimization;
    return *this;
}

sptr<Program> Compiler::Compile()
{
    sptr<VMProgram> program;

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

            std::error_code ec;
            std::filesystem::create_directories(outputPath, ec);

            if(ec)
            {
                Throw("Failed to create output path for AST: {}", ec.message());
            }

            std::filesystem::path outputFile = outputPath;
            outputFile /= "AST.txt";
            outputFile.make_preferred();

            std::ofstream stream(outputFile);
            fraze::ASTPrinter printer(stream, 2);
            printer.VisitChild(root);
        }

        SemanticAnalyzer analyzer;
        analyzer.VisitChild(root);

        if(exportLoweredCode)
        {
            std::filesystem::path outputPath = !loweredCodeOutputPath.empty() ? loweredCodeOutputPath : ".";

            std::error_code ec;
            std::filesystem::create_directories(outputPath, ec);

            if(ec)
            {
                Throw("Failed to create output path for lowered code: {}", ec.message());
            }

            auto exportFile = [&](std::string_view sourceFile, const std::string& fileName)
            {
                std::filesystem::path outputFile = outputPath;
                outputFile /= fileName;
                outputFile.make_preferred();

                std::ofstream stream(outputFile);
                fraze::CodePrinter printer(stream, 4, sourceFile);
                printer.VisitChild(root);
            };

            // one file per source file, named after its path like the bytecode export's files
            for(auto& [path, info] : sourceFiles)
                exportFile(path, utility::ReplaceAllOf(path, "\\/", '.'));

            // definitions the compiler creates without a source location, like the basic types
            exportFile("", "__global.fz");
        }

        if(codeGenerator == CodeGenerator::JIT)
        {
            JITCodeGenerator generator;
            generator.VisitChild(root);

            activeCompiler = nullptr;
            return generator.program;
        }

        VMCodeGenerator generator;
        generator.VisitChild(root);

        program = std::move(generator.program);

        activeCompiler = nullptr;

        if(exportBytecode)
        {
            std::unordered_map<std::string_view, std::vector<FunctionInfo*>> functions;
            std::vector<std::string_view> files;

            // a section's static constructor has statements from every file that adds to the section,
            // so static constructors are printed in a file of their own
            std::vector<FunctionInfo*> staticConstructors;

            for(sptr<TypeInfo>& ty : program->typeInfo)
            {
                if(FunctionInfo* funcInfo = ty->ToFunctionInfo())
                {
                    if(funcInfo->qualifiedName.ends_with(FunctionDefinition::StaticConstructorName))
                        staticConstructors.push_back(funcInfo);
                    else
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
                            ProgramDiagnostics::PrintOperation(*program, opCodeIndex, temp);
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
                                ProgramDiagnostics::PrintOperation(*program, opCodeIndex, temp);
                                fout <<  utility::ReplaceAll(temp.str(), "\"", "\"\"");
                            }
                        }

                        if(opCodeIndex == currentFunction->codeEnd)
                            currentFunction = nullptr;
                    }

                    fout << "\"" << "\n";
                }

            }
            
            // print all static constructor code
            {
                std::filesystem::path outputFile = outputPath;
                outputFile /= "static-constructors.csv";
                outputFile.make_preferred();

                std::ofstream fout(outputFile);
                std::stringstream temp;

                for(FunctionInfo* staticConstructor : staticConstructors)
                {
                    fout << "\"" << staticConstructor->qualifiedName << "\"" << ",";

                    fout << "\"";

                    size_t opCount = 0;

                    for(size_t i = staticConstructor->codeStart; i != staticConstructor->codeEnd; ++i)
                    {
                        if(opCount++ > 0)
                            fout << "\n";

                        temp.str("");
                        ProgramDiagnostics::PrintOperation(*program, i, temp);
                        fout << utility::ReplaceAll(temp.str(), "\"", "\"\"");
                    }

                    fout << "\"" << "\n";
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
    auto it = functions.find(qualifiedName);
    if(it != functions.end())
        return it->second;

    std::string nameAndSignature;
    nameAndSignature.reserve(qualifiedName.length() + 1 + signature.length());
    nameAndSignature += qualifiedName;
    nameAndSignature += ":";
    nameAndSignature += signature;

    it = functions.find(nameAndSignature);
    if(it != functions.end())
        return it->second;

    return nullptr;
}

} // fraze
