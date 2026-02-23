/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/ast/ASTRoot.h>
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/def/EnumDefinition.h>
#include <fraze/ast/def/ClassDefinition.h>
#include <fraze/ast/def/FunctionDefinition.h>
#include <fraze/ast/def/InterfaceDefinition.h>
#include <fraze/ast/def/PropertyDefinition.h>
#include <fraze/ast/def/SectionDefinition.h>
#include <fraze/ast/def/StructDefinition.h>
#include <fraze/ast/def/VariableDefinition.h>
#include <fraze/ast/stmt/VariableDefinitionStatement.h>
#include <print>

namespace fraze {

//*****************************
//  Definition
//*****************************

bool Definition::IsPartOfTemplateDeclaration()
{
    if(auto cls = this->ToClassDefinition(); cls && cls->IsTemplateDeclaration())
        return true;
    else if(auto str = this->ToStructDefinition(); str && str->IsTemplateDeclaration())
        return true;
    else if(auto itf = this->ToInterfaceDefinition(); itf && itf->IsTemplateDeclaration())
        return true;
    else if(parent && parent->IsPartOfTemplateDeclaration())
        return true;

    return false;
}

bool Definition::IsAccessibleFrom(const Scope* scope) const
{
    assert(parent->scope);

    const Scope* scopeWhereDefined = parent->scope.get();
    const Scope* scopeWhereUsed = scope;
    
    if(auto func = scopeWhereUsed->owner->ToFunctionDefinition(); func && func->isCoroutine)
    {
        if(auto classDef = func->parent->ToClassDefinition())
        {
            if(classDef->originalClassType)
            {
                auto originalClass = classDef->originalClassType->type->GetDefinition()->ToClassDefinition();

                for(const Scope* s = originalClass->scope.get(); s != nullptr; s = s->parent)
                {
                    if(s == scopeWhereDefined)
                        return true;
                }
            }

        }
    }

    for(const Scope* s = scopeWhereUsed; s != nullptr; s = s->parent)
    {
        if(s == scopeWhereDefined)
            return true;
    }

    return !IsPrivate();
}

//*****************************
//  TemplateDefinition
//*****************************

void TemplateDefinition::ApplyTemplateArgs()
{
    if(templateArgs.empty())
        return;

    ScopeStack scopes;

    auto paramDefs = GetChildren<TemplateParameterDefinition>();
    auto currentArg = templateArgs.begin();

    for(const auto& currentParam : paramDefs)
    {
        sptr<TypeSpecifier> arg = *currentArg;
        scopes.PushFromRoot(arg->scope);
        currentParam->typeSpec = arg->Clone(scopes, nullptr)->ToTypeSpecifier();
        currentParam->typeSpec->type = nullptr;
        scopes.Clear();
        ++currentArg;
    }
}

//*****************************
//  ClassDefinition
//*****************************

sptr<ASTNode> ClassDefinition::Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType)
{
    auto copyName = templateType ? templateType->GetElementTypeName() : name;
    auto copyArgs = templateType ? templateType->templateArgs : decltype(templateArgs){};
    auto copy = spnew<ClassDefinition>(loc, scopes.GetCurrent(), copyName, copyArgs);

    scopes.GetCurrent()->AddDefinition(copy);

    for(auto& itf : interfaces)
    {
        auto itfCopy = itf->Clone(scopes, nullptr)->ToTypeSpecifier();
        copy->interfaces.push_back(itfCopy);
    }

    copy->isCoroutineState = isCoroutineState;
    copy->isFunctor = isFunctor;
    copy->isExternal = isExternal;
    copy->size = size;
    copy->originalClassType = originalClassType ? originalClassType->Clone(scopes, nullptr)->ToTypeSpecifier() : decltype(originalClassType){};

    scopes.Push(copy->scope.get());

    for(auto& def : scope->definitions)
    {
        auto defCopy = def->Clone(scopes, nullptr);

        if(auto varDef = defCopy->ToVariableDefinition())
        {
            if(varDef->isStatic && !copy->IsTemplateDeclaration())
            {
                auto global = copy->GetRoot()->global;
                auto varDefStmt = spnew<VariableDefinitionStatement>(varDef->loc, global->scope.get());
                varDefStmt->variableDefinition = varDef;
                global->statements.push_back( varDefStmt );
            }
        }
    }

    scopes.Pop();

    copy->ApplyTemplateArgs();

    return copy;
}

//*****************************
//  StructDefinition
//*****************************

sptr<ASTNode> StructDefinition::Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType)
{
    auto copyName = templateType ? templateType->GetElementTypeName() : name;
    auto copyArgs = templateType ? templateType->templateArgs : decltype(templateArgs){};
    auto copy = spnew<StructDefinition>(loc, scopes.GetCurrent(), copyName, copyArgs);

    scopes.GetCurrent()->AddDefinition(copy);

    copy->size = size;

    scopes.Push(copy->scope.get());

    for(auto& def : scope->definitions)
    {
        auto defCopy = def->Clone(scopes, nullptr);

        if(auto varDef = defCopy->ToVariableDefinition())
        {
            if(varDef->isStatic && !copy->IsTemplateDeclaration())
            {
                auto global = copy->GetRoot()->global;
                auto varDefStmt = spnew<VariableDefinitionStatement>(varDef->loc, global->scope.get());
                varDefStmt->variableDefinition = varDef;
                global->statements.push_back( varDefStmt );
            }
        }
    }

    scopes.Pop();

    copy->ApplyTemplateArgs();

    return copy;
}

//*****************************
//  EnumDefinition
//*****************************

sptr<ASTNode> EnumDefinition::Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType)
{
    auto copy = spnew<EnumDefinition>(loc, scopes.GetCurrent(), name);
    scopes.GetCurrent()->AddDefinition(copy);

    scopes.Push(copy->scope.get());

    for(auto& def : scope->definitions)
        def->Clone(scopes, nullptr);

    scopes.Pop();

    return copy;
}

//*****************************
//  VariableDefinition
//*****************************

sptr<ASTNode> VariableDefinition::Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType)
{
    auto copy = spnew<VariableDefinition>(loc, scopes.GetCurrent(), typeSpec->Clone(scopes, nullptr)->ToTypeSpecifier(), name);

    scopes.GetCurrent()->AddDefinition(copy);

    copy->initializer = initializer ? initializer->Clone(scopes, nullptr)->ToExpression() : decltype(initializer){};
    copy->offset = offset;
    copy->size = size;
    copy->isStatic = isStatic;
    copy->isPrivate = isPrivate;

    return copy;
}

//*****************************
//  PropertyDefinition
//*****************************

sptr<ASTNode> PropertyDefinition::Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType)
{
    auto copy = spnew<PropertyDefinition>(loc, scopes.GetCurrent(), typeSpec->Clone(scopes, nullptr)->ToTypeSpecifier(), name);

    scopes.GetCurrent()->AddDefinition(copy);

    copy->initializer = initializer ? initializer->Clone(scopes, nullptr)->ToExpression() : decltype(initializer){};
    copy->getterName = getterName;
    copy->setterName = setterName;
    copy->offset = offset;
    copy->isStatic = isStatic;
    copy->isPrivate = isPrivate;

    return copy;
}

sptr<FunctionDefinition> PropertyDefinition::GetGetterFunction() const
{
    return !getterName.empty() ? parent->scope->FindDefinition(getterName)->ToFunctionDefinition() : nullptr;
}

sptr<FunctionDefinition> PropertyDefinition::GetSetterFunction() const
{
    return !setterName.empty() ? parent->scope->FindDefinition(setterName)->ToFunctionDefinition() : nullptr;
}

} // fraze
