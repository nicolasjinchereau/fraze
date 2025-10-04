/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <array>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <fraze/ast/ASTVisitor.h>
#include <fraze/common/Exception.h>
#include <fraze/common/Pointers.h>
#include <fraze/ast/def/SectionDefinition.h>
#include <fraze/ast/def/BasicTypeDefinition.h>

namespace fraze {

class ASTRoot : public ASTNode
{
public:
    sptr<SectionDefinition> global;

    ASTRoot(bool createGlobalSection = true)
        : ASTNode(SourceLocation())
    {
        if(createGlobalSection)
        {
            auto loc = SourceLocation();
            global = spnew<SectionDefinition>(loc, nullptr, shared_string("global"));
            global->scope->AddDefinition(spnew<BasicTypeDefinition>(loc, global->scope.get(), shared_string("$placeholder")));
            global->scope->AddDefinition(spnew<BasicTypeDefinition>(loc, global->scope.get(), shared_string("void")));
            global->scope->AddDefinition(spnew<BasicTypeDefinition>(loc, global->scope.get(), shared_string("null")));
            global->scope->AddDefinition(spnew<BasicTypeDefinition>(loc, global->scope.get(), shared_string("object")));
            global->scope->AddDefinition(spnew<BasicTypeDefinition>(loc, global->scope.get(), shared_string("bool")));
            global->scope->AddDefinition(spnew<BasicTypeDefinition>(loc, global->scope.get(), shared_string("int")));
            global->scope->AddDefinition(spnew<BasicTypeDefinition>(loc, global->scope.get(), shared_string("num")));
            global->scope->AddDefinition(spnew<BasicTypeDefinition>(loc, global->scope.get(), shared_string("string")));
            global->astRoot = this;
        }
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<ASTRoot>(false);
        copy->global = global->Clone(scopes, nullptr)->ToSectionDefinition();
        copy->global->astRoot = this;
        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<ASTRoot> ToASTRoot() override
    {
        return self();
    }
};

} // fraze
