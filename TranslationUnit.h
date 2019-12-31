/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include <vector>
#include <memory>
#include <string>
#include "ModuleDefinition.h"
#include "ASTNode.h"

class TranslationUnit : public ASTNode
{
public:
    std::string filename;
    sptr<ModuleDefinition> rootModule;

    virtual void Print(std::stringstream& stream, int indent, int tabWidth)
    {
        stream << MakeIndent(indent, tabWidth) << "TranslationUnit " << filename << std::endl;
        
        if(rootModule)
            rootModule->Print(stream, indent + 1, tabWidth);
    }
};
