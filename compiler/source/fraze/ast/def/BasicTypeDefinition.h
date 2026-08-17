/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/def/VariableDefinition.h>
#include <fraze/ast/def/FunctionDefinition.h>
#include <fraze/ast/type/Type.h>

namespace fraze {

class BasicTypeDefinition : public Definition
{
public:
    Type* type{};

    BasicTypeDefinition(const SourceLocation& loc, Scope* enclosingScope, const shared_string& name)
        : Definition(loc, enclosingScope, name)
    {
        type = Type::Get(this);
    }
    
    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<BasicTypeDefinition>(loc, scopes.GetCurrent(), name);
        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<BasicTypeDefinition> ToBasicTypeDefinition() override
    {
        return self();
    }
};

} // fraze
