/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include <memory>
#include "Statement.h"
#include "Expression.h"

class ExpressionStatement : public Statement
{
public:
    sptr<Expression> expression;

    virtual void Print(std::stringstream& stream, int indent, int tabWidth)
    {
        stream << MakeIndent(indent, tabWidth) << "ExpressionStatement" << std::endl;

        if(expression)
            expression->Print(stream, indent + 1, tabWidth);
    }
};
