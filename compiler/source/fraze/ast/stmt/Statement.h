/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/ASTNode.h>

namespace fraze {

class Statement : public ASTNode
{
public:
    Scope* enclosingScope{};

    Statement(const SourceLocation& loc, Scope* enclosingScope)
        : ASTNode(loc), enclosingScope(enclosingScope)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        ENFORCE(false, loc, "Not supported");
        return {};
    }

    virtual void Accept(ASTVisitor& visitor) override = 0;

    virtual sptr<Statement> ToStatement() override
    {
        return self();
    }
};

} // fraze
