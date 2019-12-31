/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include "Statement.h"
#include "VariableDeclaration.h"

class DeclarationStatement : public Statement
{
public:
    sptr<VariableDeclaration> variableDeclaration;

    virtual void Print(std::stringstream& stream, int indent, int tabWidth)
    {
        stream << MakeIndent(indent, tabWidth) << "DeclarationStatement" << std::endl;

        if(variableDeclaration)
            variableDeclaration->Print(stream, indent + 1, tabWidth);
    }
};
