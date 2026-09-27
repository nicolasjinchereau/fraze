/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <string_view>
#include <utility>
#include <vector>
#include <fraze/ast/AST.h>
#include <fraze/common/Pointers.h>
#include <fraze/common/Scope.h>
#include <fraze/common/SharedString.h>
#include <fraze/common/SourceLocation.h>
#include <fraze/compiler/Lexer.h>

namespace fraze {

// Builds the nodes of a lowering that's written out as statements, all at one location and scope.
struct LoweringBuilder
{
    SourceLocation loc;
    Scope* scope;

    sptr<IdentifierExpression> Identifier(std::string_view name, const sptr<Expression>& context = {}) const {
        return spnew<IdentifierExpression>(loc, scope, context, shared_string(name));
    }

    sptr<CallExpression> Call(const sptr<Expression>& context, std::string_view name, std::vector<sptr<Expression>> args = {}) const {
        return spnew<CallExpression>(loc, scope, Identifier(name, context), std::move(args));
    }

    sptr<ExpressionStatement> ExprStatement(const sptr<Expression>& expr) const {
        return spnew<ExpressionStatement>(expr, scope);
    }

    sptr<ExpressionStatement> AssignStatement(const sptr<Expression>& left, const sptr<Expression>& right) const {
        return ExprStatement(spnew<AssignExpression>(loc, scope, TokenType::Assign, left, right));
    }

    // a return that leaves a coroutine without completing it
    sptr<ReturnStatement> Return() const {
        auto ret = spnew<ReturnStatement>(loc, scope);
        ret->isCoroutineCompletion = false;
        return ret;
    }
};

} // fraze
