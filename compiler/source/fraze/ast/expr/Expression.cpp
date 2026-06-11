/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/ast/expr/Expression.h>
#include <fraze/ast/expr/IdentifierExpression.h>

namespace fraze {

bool Expression::IsValueExpression(const sptr<Expression>& expr)
{
    if (!expr)
        return false;

    auto ident = expr->ToIdentifierExpression();

    return
        ident == nullptr ||
        ident->targetDef->ToVariableDefinition() ||
        ident->targetDef->ToParameterDefinition() ||
        ident->value == "this";
}

} // fraze
