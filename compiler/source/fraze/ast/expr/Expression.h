/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/ASTNode.h>
#include <fraze/ast/type/Type.h>
#include <fraze/common/Object.h>
#include <fraze/common/Pointers.h>
#include <fraze/compiler/Compiler.h>

namespace fraze {

class Expression : public ASTNode
{
public:
    Scope* scope{}; // scope enclosing this expression
    bool pushAsRef = false; // used to detect when a struct should be pushed by reference

    Expression(const SourceLocation& loc, Scope* scope)
        : ASTNode(loc), scope(scope)
    {
        assert(scope);
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        ENFORCE(false, loc, "Not supported");
        return {};
    }

    virtual void Accept(ASTVisitor& visitor) override = 0;

    virtual sptr<Expression> ToExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() = 0;

    static bool IsValueExpression(const sptr<Expression>& expr);
};

} // fraze
