/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/ast/expr/IdentifierExpression.h>
#include <fraze/ast/def/VariableDefinition.h>
#include <fraze/ast/def/ParameterDefinition.h>
#include <fraze/ast/def/EnumDefinition.h>
#include <fraze/ast/def/EnumMemberDefinition.h>
#include <fraze/ast/def/FunctionDefinition.h>
#include <fraze/ast/def/ClassDefinition.h>
#include <fraze/ast/def/StructDefinition.h>
#include <fraze/ast/def/SectionDefinition.h>
#include <fraze/ast/def/BasicTypeDefinition.h>

namespace fraze {

Definition* IdentifierExpression::GetTargetDefinition() const {
    return targetDef;
}

Type* IdentifierExpression::EvaluateType()
{
    Type* type{};

    if(auto var = targetDef->ToVariableDefinition())
    {
        type = var->typeSpec->GetType();
    }
    else if(auto enumDef = targetDef->ToEnumDefinition())
    {
        type = enumDef->type;
    }
    else if(auto mem = targetDef->ToEnumMemberDefinition())
    {
        type = mem->parent->ToEnumDefinition()->type;
    }
    else if(auto prop = targetDef->ToPropertyDefinition())
    {
        type = prop->typeSpec->GetType();
    }
    else if(auto param = targetDef->ToParameterDefinition())
    {
        type = param->typeSpec->GetType();
    }
    else if(auto func = targetDef->ToFunctionDefinition())
    {
        type = func->type;
    }
    else if(auto cls = targetDef->ToClassDefinition())
    {
        type = cls->type;
    }
    else if(auto str = targetDef->ToStructDefinition())
    {
        type = str->type;
    }
    else if(auto itf = targetDef->ToInterfaceDefinition())
    {
        type = itf->type;
    }
    else if(auto sec = targetDef->ToSectionDefinition())
    {
        type = sec->type;
    }
    else
    {
        // refers to a type directly and cannot be evaluated
        type = nullptr;
    }

    return type;
}

} // fraze
