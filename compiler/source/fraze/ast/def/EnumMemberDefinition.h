/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/type/Type.h>
#include <fraze/common/Object.h>
#include <fraze/common/Scope.h>

namespace fraze {

class EnumMemberDefinition : public Definition
{
public:
    sptr<Expression> value;

    EnumMemberDefinition(
        const SourceLocation& loc
        , Scope* enclosingScope
        , const shared_string& name)
        : Definition(loc, enclosingScope, name)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<EnumMemberDefinition>(loc, scopes.GetCurrent(), name);
        scopes.GetCurrent()->AddDefinition(copy);
        
        copy->value = value->Clone(scopes, nullptr)->ToExpression();

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<EnumMemberDefinition> ToEnumMemberDefinition() override
    {
        return self();
    }


};

} // fraze
