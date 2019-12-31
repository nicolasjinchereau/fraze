/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include "Expression.h"

class IntegerExpression : public Expression
{
public:
    int value;

    IntegerExpression(int value = 0)
        : value(value)
    {
    }

    virtual void Print(std::stringstream& stream, int indent, int tabWidth)
    {
        stream << MakeIndent(indent, tabWidth) << "IntegerExpression " << value <<  std::endl;
    }
};
