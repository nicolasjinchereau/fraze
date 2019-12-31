/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include <string>
#include "ASTNode.h"
#include "Expression.h"

class VariableDeclaration : public ASTNode
{
public:
    std::string typeName;
    std::string id;
    sptr<Expression> initializer;

    virtual void Print(std::stringstream& stream, int indent, int tabWidth)
    {
        stream << MakeIndent(indent, tabWidth) << "VariableDeclaration " << typeName << " " << id << std::endl;

        if (initializer)
            initializer->Print(stream, indent + 1, tabWidth);
    }
};
