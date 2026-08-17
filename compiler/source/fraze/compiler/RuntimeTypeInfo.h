/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <vector>
#include <unordered_map>
#include <fraze/ast/AST.h>
#include <fraze/common/Pointers.h>
#include <fraze/program/Program.h>

namespace fraze {

class RuntimeTypeInfo
{
public:
    const TypeInfo* typeInfoTypeInfo{};
    std::vector<sptr<TypeInfo>> allTypeInfo;
    std::unordered_map<const Type*, sptr<TypeInfo>> typeInfoByType;
    std::vector<IntrinsicFunction> intrinsics;
    size_t globalSize{};

    RuntimeTypeInfo(const sptr<ASTRoot>& root, const std::vector<sptr<Type>>& types);

private:

    static void CalculateSizesAndOffsets(const sptr<Definition>& def);

    static size_t GetTypeSize(Type* type);

    static size_t GetVariableSize(const sptr<VariableDefinition>& varDef);
    static size_t GetParameterSize(const sptr<ParameterDefinition>& paramDef);
    static size_t GetStructSize(const sptr<StructDefinition>& structDef);
    static size_t GetClassSize(const sptr<ClassDefinition>& classDef);
    static size_t GetGlobalSize(const sptr<Definition>& def);

    static size_t GetParameterOffset(const sptr<ParameterDefinition>& node);
    static size_t GetVariableOffset(const sptr<VariableDefinition>& node);
    static size_t GetFunctionOffset(const sptr<FunctionDefinition>& node);
    static size_t GetPropertyOffset(const sptr<PropertyDefinition>& node);

    static size_t GetLocalStorageSize(const sptr<FunctionDefinition>& func);
    static size_t GetParamListSize(const sptr<FunctionDefinition>& func);

    sptr<TypeInfo> GetTypeInfo(Type* type);

};

} // fraze
