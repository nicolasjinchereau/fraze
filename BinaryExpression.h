/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include "Expression.h"
#include "Lexer.h"

class BinaryExpression : public Expression
{
public:
    TokenType operation;
    sptr<Expression> left;
    sptr<Expression> right;

    BinaryExpression(TokenType op, const sptr<Expression>& left, const sptr<Expression>& right)
        : operation(op), left(left), right(right)
    {
    }

    virtual void Print(std::stringstream& stream, int indent, int tabWidth)
    {
        stream << MakeIndent(indent, tabWidth) << "BinaryExpression " << Lexer::GetTokenName(operation) << std::endl;

        left->Print(stream, indent + 1, tabWidth);
        right->Print(stream, indent + 1, tabWidth);
    }
};
