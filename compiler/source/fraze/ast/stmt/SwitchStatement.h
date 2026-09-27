/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <optional>
#include <vector>
#include <fraze/ast/def/EnumMemberDefinition.h>
#include <fraze/ast/expr/IdentifierExpression.h>
#include <fraze/ast/expr/IntegerLiteralExpression.h>
#include <fraze/ast/stmt/BlockStatement.h>
#include <fraze/ast/stmt/Statement.h>

namespace fraze {

class SwitchStatement : public Statement
{
public:
    // Consecutive case labels share one section, whose body runs until the next label.
    // Sections don't fall through into each other.
    struct Section
    {
        std::vector<sptr<Expression>> caseValues;
        bool isDefault = false;
        sptr<BlockStatement> body;
    };

    sptr<Expression> value;
    std::vector<Section> sections;

    SwitchStatement(const SourceLocation& loc, Scope* enclosingScope)
        : Statement(loc, enclosingScope)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<SwitchStatement>(loc, scopes.GetCurrent());

        copy->value = value->Clone(scopes, nullptr)->ToExpression();

        for(auto& section : sections)
        {
            Section& sectionCopy = copy->sections.emplace_back();

            for(auto& caseValue : section.caseValues)
                sectionCopy.caseValues.push_back(caseValue->Clone(scopes, nullptr)->ToExpression());

            sectionCopy.isDefault = section.isDefault;
            sectionCopy.body = section.body->Clone(scopes, nullptr)->ToBlockStatement();
        }

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<SwitchStatement> ToSwitchStatement() override
    {
        return self();
    }

    // The value of a case label, which has to be an integer literal or an enum member. Returns
    // nothing for anything else, or for an enum member not yet resolved.
    static std::optional<int64_t> GetCaseValue(const sptr<Expression>& caseValue)
    {
        if(auto literal = caseValue->ToIntegerLiteralExpression())
            return literal->value;

        if(auto ident = caseValue->ToIdentifierExpression(); ident && ident->targetDef)
        {
            if(auto enumMember = ident->targetDef->ToEnumMemberDefinition())
                return enumMember->value->ToIntegerLiteralExpression()->value;
        }

        return std::nullopt;
    }
};

} // fraze
