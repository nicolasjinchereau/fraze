/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include <vector>
#include "Statement.h"

class BlockStatement : public Statement
{
public:
    std::vector<sptr<Statement>> statements;

    virtual void Print(std::stringstream& stream, int indent, int tabWidth)
    {
        stream << MakeIndent(indent, tabWidth) << "BlockStatement" << std::endl;

        for (auto& s : statements)
            s->Print(stream, indent + 1, tabWidth);
    }
};
