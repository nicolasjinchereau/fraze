/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include <string>
#include <vector>
#include <memory>
#include "ASTVisitor.h"
#include <sstream>

template<class T>
using sptr = std::shared_ptr<T>;

class ASTNode
{
public:
    std::string MakeIndent(int indent, int tabWidth) {
        return std::string(indent * tabWidth, ' ');
    }

    virtual void Print(std::stringstream& stream, int indent = 0, int tabWidth = 4) {
        stream << MakeIndent(indent, tabWidth) << "ASTNode" << std::endl;
    }

    virtual void Accept(ASTVisitor* v) {
        v->Visit(this);
    }
};
