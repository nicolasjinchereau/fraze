/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/def/VariableDefinition.h>
#include <fraze/ast/stmt/Statement.h>

namespace fraze {

// TODO: rename to VariableInitStatement
class VariableDefinitionStatement : public Statement
{
public:
    sptr<VariableDefinition> variableDefinition;

    VariableDefinitionStatement(const SourceLocation& loc, Scope* enclosingScope)
        : Statement(loc, enclosingScope)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<VariableDefinitionStatement>(loc, scopes.GetCurrent());

        copy->variableDefinition = variableDefinition->Clone(scopes, nullptr)->ToVariableDefinition();

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<VariableDefinitionStatement> ToVariableDefinitionStatement() override
    {
        return self();
    }
};

} // fraze
