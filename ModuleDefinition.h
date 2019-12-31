/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include <vector>
#include <string>
#include <memory>
#include "FunctionDefinition.h"
#include "VariableDeclaration.h"
#include "ASTNode.h"

class ModuleDefinition : public ASTNode
{
public:
    std::string id;
    std::vector<sptr<VariableDeclaration>> variables;
    std::vector<sptr<FunctionDefinition>> functions;
    std::vector<sptr<ModuleDefinition>> modules;

    ModuleDefinition() {}

    ModuleDefinition(const std::string& id)
        : id(id) {}

    virtual void Print(std::stringstream& stream, int indent, int tabWidth)
    {
        stream << MakeIndent(indent, tabWidth) << "ModuleDefinition " << id << std::endl;

        for (auto& v : variables)
            v->Print(stream, indent + 1, tabWidth);

        for (auto& f : functions)
            f->Print(stream, indent + 1, tabWidth);

        for (auto& m : modules)
            m->Print(stream, indent + 1, tabWidth);
    }
};
