/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <string>
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/expr/Expression.h>
#include <fraze/ast/type/TypeSpecifier.h>

namespace fraze {

class ClassDefinition;

class VariableDefinition : public Definition
{
public:
    sptr<TypeSpecifier> typeSpec;
    sptr<Expression> initializer;
    size_t offset = 0;
    size_t size = 0;
    // TODO: hook this up in the parser so it follows the same convention as functions and properties
    //bool isMember = false;
    bool isStatic = false;
    bool isPrivate = false;
    Type* fieldType{};

    VariableDefinition(const SourceLocation& loc, Scope* enclosingScope, const sptr<TypeSpecifier>& typeSpec, const shared_string& name)
        : Definition(loc, enclosingScope, name), typeSpec(typeSpec)
    {
        auto owner = enclosingScope->owner;
        if((owner->ToClassDefinition() || owner->ToStructDefinition()) && !owner->IsPartOfTemplateDeclaration())
        {
            fieldType = Type::Get(this);
        }
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override;

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<VariableDefinition> ToVariableDefinition() override
    {
        return self();
    }

    virtual bool IsPrivate() const override {
        return isPrivate;
    }
};

} // fraze
