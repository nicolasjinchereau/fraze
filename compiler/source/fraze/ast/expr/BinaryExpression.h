/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/expr/Expression.h>
#include <fraze/compiler/Lexer.h>

namespace fraze {

class BinaryExpression : public Expression
{
public:
    TokenType operation;
    sptr<Expression> left;
    sptr<Expression> right;

    BinaryExpression(const SourceLocation& loc, Scope* scope, TokenType op, const sptr<Expression>& left, const sptr<Expression>& right)
        : Expression(loc, scope), operation(op), left(left), right(right)
    {
    }

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType) override
    {
        auto copy = spnew<BinaryExpression>(loc, scopes.GetCurrent(), operation,
                left->Clone(scopes, nullptr)->ToExpression(),
                right->Clone(scopes, nullptr)->ToExpression());
        
        copy->pushAsRef = pushAsRef;

        return copy;
    }

    virtual void Accept(ASTVisitor& visitor) override
    {
        visitor.Visit(self());
    }

    virtual sptr<BinaryExpression> ToBinaryExpression() override
    {
        return self();
    }

    virtual Type* EvaluateType() override
    {
        if( operation == TokenType::Equal ||
            operation == TokenType::NotEqual ||
            operation == TokenType::Less ||
            operation == TokenType::LessEqual ||
            operation == TokenType::Greater ||
            operation == TokenType::GreaterEqual ||
            operation == TokenType::LogicalAnd ||
            operation == TokenType::LogicalOr ||
            operation == TokenType::LogicalNot ||
            operation == TokenType::BitTest)
        {
            return Type::Get("bool");
        }
        else
        {
            return left->EvaluateType();
        }
    }
};

} // fraze
