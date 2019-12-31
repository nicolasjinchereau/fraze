/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include "Expression.h"
#include <string>
#include <vector>

class FunctionExpression : public Expression
{
public:
    std::string name;
    std::vector<sptr<Expression>> arguments;

    virtual void Print(std::stringstream& stream, int indent, int tabWidth)
    {
        stream << MakeIndent(indent, tabWidth) << "FunctionExpression " << name << std::endl;

        for(auto& a : arguments)
            a->Print(stream, indent + 1, tabWidth);
    }
};
