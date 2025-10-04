/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <vector>
#include <fraze/ast/stmt/Statement.h>
#include <fraze/common/Scope.h>

namespace fraze {

class BlockStatement : public Statement
{
public:
    sptr<Scope> scope;
    std::vector<sptr<Statement>> statements;

    BlockStatement(const SourceLocation& loc, Scope* enclosingScope)
        : Statement(loc, enclosingScope)
    {
        scope = spnew<Scope>();
        scope->parent = enclosingScope;
        scope->owner = enclosingScope->owner;
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<BlockStatement>(loc, scopes.GetCurrent());

        scopes.Push(copy->scope.get());

        for(auto& stmt : statements)
        {
            auto stmtCopy = stmt->Clone(scopes, nullptr)->ToStatement();
            copy->statements.push_back(stmtCopy);
        }

        scopes.Pop();

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }
    
    virtual sptr<BlockStatement> ToBlockStatement() override
    {
        return self();
    }
};

} // fraze
