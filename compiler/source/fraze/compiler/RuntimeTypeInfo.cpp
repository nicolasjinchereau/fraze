/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/compiler/RuntimeTypeInfo.h>
#include <fraze/compiler/Compiler.h>

namespace fraze {

RuntimeTypeInfo::RuntimeTypeInfo(const sptr<ASTRoot>& root, const std::vector<sptr<Type>>& types)
{
    globalSize = GetGlobalSize(root->global);
    CalculateSizesAndOffsets(root->global);

    for(auto& type : types)
    {
        // skip template declarations
        auto def = type->GetDefinition();
        if(def && def->IsPartOfTemplateDeclaration())
            continue;

        (void)GetTypeInfo(type.get());
    }
}

void RuntimeTypeInfo::CalculateSizesAndOffsets(const sptr<Definition>& def)
{
    if(def->IsPartOfTemplateDeclaration())
        return;

    if(auto varDef = def->ToVariableDefinition())
    {
        varDef->size = GetVariableSize(varDef);
        varDef->offset = GetVariableOffset(varDef);
    }
    else if(auto paramDef = def->ToParameterDefinition())
    {
        paramDef->size = GetParameterSize(paramDef);
        paramDef->offset = GetParameterOffset(paramDef);
    }
    else if(auto propDef = def->ToPropertyDefinition())
    {
        propDef->offset = GetPropertyOffset(propDef);
    }
    else if(auto funcDef = def->ToFunctionDefinition())
    {
        funcDef->offset = GetFunctionOffset(funcDef);
        funcDef->paramSize = GetParamListSize(funcDef);
        funcDef->localSize = GetLocalStorageSize(funcDef);
    }
    else if(auto structDef = def->ToStructDefinition())
    {
        structDef->size = GetStructSize(structDef);
    }
    else if(auto classDef = def->ToClassDefinition())
    {
        classDef->size = GetClassSize(classDef);
    }

    for(auto& child : def->children)
    {
        CalculateSizesAndOffsets(child);
    }
}

size_t RuntimeTypeInfo::GetTypeSize(Type* type)
{
    size_t size = 1;

    if(auto def = type->GetDefinition())
    {
        if(auto structDef = def->ToStructDefinition())
            size = GetStructSize(structDef);
    }

    return size;
}

size_t RuntimeTypeInfo::GetVariableSize(const sptr<VariableDefinition>& varDef)
{
    return GetTypeSize(varDef->typeSpec->type);
}

size_t RuntimeTypeInfo::GetParameterSize(const sptr<ParameterDefinition>& paramDef)
{
    return GetTypeSize(paramDef->typeSpec->type);
}

size_t RuntimeTypeInfo::GetStructSize(const sptr<StructDefinition>& structDef)
{
    size_t size = 0;

    for(const auto& field : structDef->GetChildren<VariableDefinition>())
        size += GetVariableSize(field);

    return size;
}

size_t RuntimeTypeInfo::GetClassSize(const sptr<ClassDefinition>& classDef)
{
    size_t size = 0;

    for(const auto& field : classDef->GetChildren<VariableDefinition>())
        size += GetVariableSize(field);

    return size;
}

size_t RuntimeTypeInfo::GetGlobalSize(const sptr<Definition>& def)
{
    size_t offset = 0;

    if(!def->IsPartOfTemplateDeclaration())
    {
        if(const auto& varDef = def->ToVariableDefinition())
        {
            if(varDef->isStatic)
                offset += GetVariableSize(varDef);
        }
        else
        {
            for(auto& child : def->children)
                offset += GetGlobalSize(child);
        }
    }

    return offset;
};

size_t RuntimeTypeInfo::GetParameterOffset(const sptr<ParameterDefinition>& node)
{
    size_t offset = 0;

    for(const auto& paramDef : node->parent->GetChildren<ParameterDefinition>())
    {
        if(paramDef == node)
            break;

        offset += GetParameterSize(paramDef);
    }

    return offset;
}

size_t RuntimeTypeInfo::GetVariableOffset(const sptr<VariableDefinition>& node)
{
    size_t offset = 0;

    if(node->isStatic)
    {
        struct S
        {
            static bool GetGlobalOffset(
                const sptr<Definition>& def,
                const sptr<Definition>& target,
                size_t& offset)
            {
                if(def == target)
                    return true;

                if(def->IsPartOfTemplateDeclaration())
                    return false;

                if(const auto& varDef = def->ToVariableDefinition())
                {
                    if(varDef->isStatic)
                        offset += GetVariableSize(varDef);
                }
                else
                {
                    for(auto& child : def->children)
                    {
                        if(GetGlobalOffset(child, target, offset))
                            return true;
                    }
                }

                return false;
            };
        };

        bool found = S::GetGlobalOffset(node->GetRoot()->global, node, offset);
        assert(found);
    }
    else
    {
        for(const auto& varDef : node->parent->GetChildren<VariableDefinition>())
        {
            if(varDef == node)
                break;

            offset += GetVariableSize(varDef);
        }
    }

    return offset;
}

size_t RuntimeTypeInfo::GetFunctionOffset(const sptr<FunctionDefinition>& node)
{
    size_t offset = 0;

    for(const auto& func : node->parent->GetChildren<FunctionDefinition>())
    {
        if(func == node)
            break;

        ++offset;
    }

    return offset;
}

size_t RuntimeTypeInfo::GetPropertyOffset(const sptr<PropertyDefinition>& node)
{
    size_t offset = 0;

    for(const auto& prop : node->parent->GetChildren<PropertyDefinition>())
    {
        if(prop == node)
            break;

        ++offset;
    }

    return offset;
}

size_t RuntimeTypeInfo::GetLocalStorageSize(const sptr<FunctionDefinition>& func)
{
    size_t localSize = 0;

    for(const auto& local : func->GetChildren<VariableDefinition>())
        localSize += GetVariableSize(local);

    return localSize;
}

size_t RuntimeTypeInfo::GetParamListSize(const sptr<FunctionDefinition>& func)
{
    size_t paramSize = 0;

    for(const auto& param : func->GetChildren<ParameterDefinition>())
        paramSize += GetParameterSize(param);

    return paramSize;
}

sptr<TypeInfo> RuntimeTypeInfo::GetTypeInfo(Type* type)
{
    auto it = typeInfoByType.find(type);
    if(it != typeInfoByType.end())
        return it->second;

    sptr<TypeInfo> ret;

    if(type->IsArray())
    {
        sptr<ArrayInfo> info = spnew<ArrayInfo>();
        info->id = allTypeInfo.size();
        info->qualifiedName = type->GetName();
        info->loc = SourceLocation();
        allTypeInfo.push_back(info);
        typeInfoByType[type] = info;
        ret = info;

        info->elementType = GetTypeInfo(type->GetElementType()).get();
    }
    else
    {
        auto def = type->GetDefinition();

        if(auto basicType = def->ToBasicTypeDefinition())
        {
            sptr<BasicTypeInfo> info = spnew<BasicTypeInfo>();
            info->id = allTypeInfo.size();
            info->qualifiedName = type->GetName();
            info->loc = basicType->loc;
            allTypeInfo.push_back(info);
            typeInfoByType[type] = info;
            ret = info;
        }
        else if(auto classDef = def->ToClassDefinition())
        {
            sptr<ClassInfo> info = spnew<ClassInfo>();
            info->id = allTypeInfo.size();
            info->qualifiedName = type->GetName();
            info->loc = classDef->loc;
            info->size = classDef->size;
            allTypeInfo.push_back(info);
            typeInfoByType[type] = info;
            ret = info;

            for(auto& itf : classDef->interfaces)
            {
                auto itfTypeInfo = GetTypeInfo(itf->type);
                info->interfaces.push_back(itfTypeInfo->id);

                auto& impl = classDef->implementations[info->implementations.size()];
                info->implementations.emplace_back();

                for(auto* funcDef : impl)
                {
                    auto funcTypeInfo = GetTypeInfo(funcDef->type);
                    info->implementations.back().push_back(funcTypeInfo->id);
                }
            }

            for(const auto& field : classDef->GetChildren<VariableDefinition>())
            {
                assert(field->fieldType);
                auto fieldTypeInfo = GetTypeInfo(field->fieldType);
                info->fields.push_back(fieldTypeInfo->ToFieldInfo());
            }
        }
        else if(auto interfaceDef = def->ToInterfaceDefinition())
        {
            sptr<InterfaceInfo> info = spnew<InterfaceInfo>();
            info->id = allTypeInfo.size();
            info->qualifiedName = type->GetName();
            info->loc = interfaceDef->loc;
            allTypeInfo.push_back(info);
            typeInfoByType[type] = info;
            ret = info;
        }
        else if(auto structDef = def->ToStructDefinition())
        {
            sptr<StructInfo> info = spnew<StructInfo>();
            info->id = allTypeInfo.size();
            info->qualifiedName = type->GetName();
            info->loc = structDef->loc;
            info->size = structDef->size;
            allTypeInfo.push_back(info);
            typeInfoByType[type] = info;
            ret = info;

            for(const auto& field : structDef->GetChildren<VariableDefinition>())
            {
                assert(field->fieldType);
                auto fieldTypeInfo = GetTypeInfo(field->fieldType);
                info->fields.push_back(fieldTypeInfo->ToFieldInfo());
            }
        }
        else if(auto varDef = def->ToVariableDefinition())
        {
            auto owner = varDef->enclosingScope->owner;
            assert(owner->ToClassDefinition() || owner->ToStructDefinition());

            sptr<FieldInfo> info = spnew<FieldInfo>();
            info->id = allTypeInfo.size();
            info->qualifiedName = type->GetName();
            info->loc = varDef->loc;
            info->offset = varDef->offset;
            allTypeInfo.push_back(info);
            typeInfoByType[type] = info;
            ret = info;

            info->type = GetTypeInfo(varDef->typeSpec->type).get();
        }
        else if(auto enumDef = def->ToEnumDefinition())
        {
            sptr<EnumInfo> info = spnew<EnumInfo>();
            info->id = allTypeInfo.size();
            info->qualifiedName = type->GetName();
            info->loc = enumDef->loc;

            for(const auto& child : enumDef->GetChildren<EnumMemberDefinition>())
            {
                auto value = child->value->ToIntegerLiteralExpression()->value;
                info->members.push_back({ std::string(child->name), value });
            }

            allTypeInfo.push_back(info);
            typeInfoByType[type] = info;
            ret = info;
        }
        else if(auto func = def->ToFunctionDefinition())
        {
            size_t returnSize = 1;

            if(func->returnType->type->IsStruct())
            {
                auto structDef = func->returnType->type->GetDefinition()->ToStructDefinition();
                returnSize = structDef->size;
                assert(returnSize >= 1);
            }

            // param offset and size
            std::vector<ParamInfo> params;
            for(const auto& param : func->GetChildren<ParameterDefinition>())
            {
                params.push_back({ (uint32_t)param->offset, (uint32_t)param->size });
            }

            int intrinsicID = -1;

            if(func->externalIntrinsic)
            {
                for(size_t i = 0; i != intrinsics.size(); ++i)
                {
                    if(intrinsics[i] == func->externalIntrinsic)
                    {
                        intrinsicID = (int)i;
                        break;
                    }
                }

                if(intrinsicID == -1)
                {
                    intrinsicID = (int)intrinsics.size();
                    intrinsics.push_back(func->externalIntrinsic);
                }
            }

            sptr<FunctionInfo> info = spnew<FunctionInfo>();
            info->id = allTypeInfo.size();
            info->qualifiedName = type->GetName();
            info->loc = func->loc;
            info->returnSize = (uint32_t)returnSize;
            info->paramSize = (uint32_t)func->paramSize;
            info->localSize = (uint32_t)func->localSize;
            info->codeStart = 0;
            info->codeEnd = 0;
            info->externalFunction = func->externalFunction;
            info->intrinsicID = intrinsicID;
            info->hasContext = !func->isStatic;
            info->isExternal = func->isExternal;
            info->offset = static_cast<uint32_t>(func->offset);
            info->params = std::move(params);
            allTypeInfo.push_back(info);
            typeInfoByType[type] = info;
            ret = info;
        }
        else if(auto sect = def->ToSectionDefinition())
        {
            sptr<SectionInfo> info = spnew<SectionInfo>();
            info->id = allTypeInfo.size();
            info->qualifiedName = type->GetName();
            info->loc = sect->loc;
            info->codeStart = 0;
            info->codeEnd = 0;
            allTypeInfo.push_back(info);
            typeInfoByType[type] = info;
            ret = info;
        }
    }

    return ret;
}


} // fraze
