/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/ASTNode.h>

#include <fraze/ast/ASTRoot.h>

#include <fraze/ast/def/BasicTypeDefinition.h>
#include <fraze/ast/def/ClassDefinition.h>
#include <fraze/ast/def/Definition.h>
#include <fraze/ast/def/EnumDefinition.h>
#include <fraze/ast/def/EnumMemberDefinition.h>
#include <fraze/ast/def/FunctionDefinition.h>
#include <fraze/ast/def/InterfaceDefinition.h>
#include <fraze/ast/def/ParameterDefinition.h>
#include <fraze/ast/def/PropertyDefinition.h>
#include <fraze/ast/def/SectionDefinition.h>
#include <fraze/ast/def/StructDefinition.h>
#include <fraze/ast/def/TemplateDefinition.h>
#include <fraze/ast/def/TemplateParameterDefinition.h>
#include <fraze/ast/def/VariableDefinition.h>

#include <fraze/ast/expr/AsExpression.h>
#include <fraze/ast/expr/AssignExpression.h>
#include <fraze/ast/expr/AwaitExpression.h>
#include <fraze/ast/expr/BinaryExpression.h>
#include <fraze/ast/expr/BooleanLiteralExpression.h>
#include <fraze/ast/expr/CachedExpression.h>
#include <fraze/ast/expr/CallExpression.h>
#include <fraze/ast/expr/ConvertExpression.h>
#include <fraze/ast/expr/DefaultValueExpression.h>
#include <fraze/ast/expr/EmitExpression.h>
#include <fraze/ast/expr/Expression.h>
#include <fraze/ast/expr/FoldExpression.h>
#include <fraze/ast/expr/IdentifierExpression.h>
#include <fraze/ast/expr/IndexExpression.h>
#include <fraze/ast/expr/IntegerLiteralExpression.h>
#include <fraze/ast/expr/IsExpression.h>
#include <fraze/ast/expr/NewExpression.h>
#include <fraze/ast/expr/NullLiteralExpression.h>
#include <fraze/ast/expr/NumberLiteralExpression.h>
#include <fraze/ast/expr/PostfixExpression.h>
#include <fraze/ast/expr/PrefixExpression.h>
#include <fraze/ast/expr/SizeOfExpression.h>
#include <fraze/ast/expr/StringLiteralExpression.h>
#include <fraze/ast/expr/TernaryExpression.h>

#include <fraze/ast/type/TypeSpecifier.h>

#include <fraze/ast/stmt/AssertStatement.h>
#include <fraze/ast/stmt/BlockStatement.h>
#include <fraze/ast/stmt/ExposeStatement.h>
#include <fraze/ast/stmt/ExpressionStatement.h>
#include <fraze/ast/stmt/ForStatement.h>
#include <fraze/ast/stmt/GotoStatement.h>
#include <fraze/ast/stmt/IfStatement.h>
#include <fraze/ast/stmt/ReturnStatement.h>
#include <fraze/ast/stmt/Statement.h>
#include <fraze/ast/stmt/VariableDefinitionStatement.h>
#include <fraze/ast/stmt/WhileStatement.h>
