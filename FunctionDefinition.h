/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include <string>
#include <vector>
#include "FunctionParameter.h"
#include "Statement.h"
#include "ASTNode.h"

class FunctionDefinition : public ASTNode
{
public:
    std::string returnTypeName;
    std::string name;
    std::vector<sptr<FunctionParameter>> params;
    sptr<Statement> body;

    virtual void Print(std::stringstream& stream, int indent, int tabWidth)
    {
        stream << MakeIndent(indent, tabWidth) << "FunctionDefinition " << returnTypeName << " " << name << std::endl;
        
        for (auto& p : params)
            p->Print(stream, indent + 1, tabWidth);

        if(body)
            body->Print(stream, indent + 1, tabWidth);
    }
};
