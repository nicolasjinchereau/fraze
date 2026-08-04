/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <string>
#include <vector>
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/def/ParameterDefinition.h>
#include <fraze/ast/expr/IdentifierExpression.h>
#include <fraze/ast/stmt/BlockStatement.h>
#include <fraze/ast/type/TypeSpecifier.h>
#include <fraze/common/ExternalFunction.h>
#include <fraze/common/Scope.h>

namespace fraze {

class ClassDefinition;

class FunctionDefinition : public Definition
{
public:
    sptr<TypeSpecifier> returnType;

    bool isExternal = false;
    bool isAbstract = false;
    bool isStatic = false;
    bool isPrivate = false;
    bool isMember = false;
    bool isCoroutine = false;
    bool isConstructor = false;
    sptr<IExternalFunction> externalFunction;
    IntrinsicFunction externalIntrinsic{};
    sptr<BlockStatement> body;
    size_t offset = 0;
    size_t paramSize = 0;
    size_t localSize = 0;
    Type* type{};

    FunctionDefinition(const SourceLocation& loc, Scope* enclosingScope, const shared_string& name)
        : Definition(loc, enclosingScope, name)
    {
        scope = spnew<Scope>();
        scope->parent = enclosingScope;
        scope->owner = this;
        type = Type::Get(this);
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<FunctionDefinition>(loc, scopes.GetCurrent(), name);

        scopes.GetCurrent()->AddDefinition(copy);

        copy->returnType = returnType ? returnType->Clone(scopes, nullptr)->ToTypeSpecifier() : decltype(returnType){};

        scopes.Push(copy->scope.get());

        for(auto& def : scope->definitions)
            def->Clone(scopes, nullptr);

        copy->isExternal = isExternal;
        copy->isAbstract = isAbstract;
        copy->isStatic = isStatic;
        copy->isPrivate = isPrivate;
        copy->isMember = isMember;
        copy->isCoroutine = isCoroutine;
        copy->isConstructor = isConstructor;
        copy->externalFunction = externalFunction;
        copy->offset = offset;
        copy->paramSize = paramSize;
        copy->localSize = localSize;

        copy->body = body ? body->Clone(scopes, nullptr)->ToBlockStatement() : decltype(body){};

        scopes.Pop();

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<FunctionDefinition> ToFunctionDefinition() override
    {
        return self();
    }

    virtual bool IsPrivate() const override {
        return isPrivate;
    }

    bool HasImplicitThisParam() const
    {
        bool hasImplicitThis = isMember && !isStatic && !(isExternal && isConstructor);
        assert(!hasImplicitThis || GetFirstChild<ParameterDefinition>( "this" ) != nullptr);
        return hasImplicitThis;
    }
};

} // fraze
