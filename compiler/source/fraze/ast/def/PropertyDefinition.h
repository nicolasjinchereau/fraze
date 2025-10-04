/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <string>
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/expr/Expression.h>
#include <fraze/ast/type/TypeSpecifier.h>
#include <fraze/common/SharedString.h>

namespace fraze {

class ClassDefinition;
class FunctionDefinition;

class PropertyDefinition : public Definition
{
public:
    sptr<TypeSpecifier> typeSpec;
    sptr<Expression> initializer;
    shared_string getterName;
    shared_string setterName;
    size_t offset = 0;
    bool isStatic = false;
    bool isPrivate = false;

    PropertyDefinition(const SourceLocation& loc, Scope* enclosingScope, const sptr<TypeSpecifier>& typeSpec, const shared_string& name)
        : Definition(loc, enclosingScope, name), typeSpec(typeSpec)
    {
        
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override;

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<PropertyDefinition> ToPropertyDefinition() override
    {
        return self();
    }

    sptr<FunctionDefinition> GetGetterFunction() const;
    sptr<FunctionDefinition> GetSetterFunction() const;

    virtual bool IsPrivate() const override {
        return isPrivate;
    }
};

} // fraze
