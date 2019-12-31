/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include "ASTNode.h"

class ASTNode;
class BlockStatement;

class ASTVisitor
{
public:
    virtual void Visit(ASTNode* node) {}
    virtual void Visit(BlockStatement* node) {}
};
