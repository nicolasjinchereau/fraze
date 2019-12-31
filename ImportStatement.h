/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include "ASTNode.h"

class ImportStatement : public ASTNode
{
public:
    virtual void Print(std::stringstream& stream, int indent, int tabWidth)
    {
        stream << MakeIndent(indent, tabWidth) << "ImportStatement" << std::endl;
    }
};
