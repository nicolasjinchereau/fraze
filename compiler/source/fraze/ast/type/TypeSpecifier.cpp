/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/ast/type/TypeSpecifier.h>
#include <fraze/ast/def/TemplateDefinition.h>
#include <fraze/common/Pointers.h>

namespace fraze {

TypeSpecifier::TypeSpecifier(const SourceLocation& loc, Type* type)
    : ASTNode(loc)
    , type(type)
{
    while(type->IsArray())
    {
        ++arrayDimensions;
        type = type->GetElementType();
    }

    auto def = type->GetDefinition();

    std::string_view name = def->name;
    baseTypeName = shared_string(name.substr(0, name.find('<')));
    scope = def->enclosingScope;

    if(auto templateDef = def->ToTemplateDefinition())
    {
        for(auto& arg : templateDef->templateArgs)
        {
            templateArgs.push_back(spnew<TypeSpecifier>(*arg));
        }
    }
}

TypeSpecifier::TypeSpecifier(const TypeSpecifier& other)
    : ASTNode(other.loc)
    , scope(other.scope)
    , baseTypeName(other.baseTypeName)
    , arrayDimensions(other.arrayDimensions)
{
    for(auto& arg : other.templateArgs)
    {
        templateArgs.push_back(spnew<TypeSpecifier>(*arg));
    }
}

} // fraze
