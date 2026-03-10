/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <array>
#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <fraze/ast/ASTVisitor.h>
#include <fraze/common/Exception.h>
#include <fraze/common/Pointers.h>
#include <fraze/common/Scope.h>

namespace fraze {

class ASTRoot;
class TypeSpecifier;

class ASTNode : public sptr_from_this<ASTNode>
{
public:
    SourceLocation loc;

    ASTNode(const SourceLocation& loc)
        : loc(loc)
    {
    }

    ASTNode(const ASTNode&) = default;

    virtual ~ASTNode(){}

    virtual sptr<ASTNode> Clone(ScopeStack& scopes, const sptr<TypeSpecifier>& templateType = {})
    {
        ENFORCE(false, loc, "Not supported");
        return {};
    }

    virtual void Accept(ASTVisitor& visitor) = 0;

    virtual sptr<ASTRoot> ToASTRoot(){ return {}; }

    virtual sptr<BasicTypeDefinition> ToBasicTypeDefinition(){ return {}; }
    virtual sptr<ClassDefinition> ToClassDefinition(){ return {}; }
    virtual sptr<Definition> ToDefinition(){ return {}; }
    virtual sptr<EnumDefinition> ToEnumDefinition(){ return {}; }
    virtual sptr<EnumMemberDefinition> ToEnumMemberDefinition(){ return {}; }
    virtual sptr<InterfaceDefinition> ToFunctorInterfaceDefinition(){ return {}; }
    virtual sptr<ClassDefinition> ToFunctorClassDefinition(){ return {}; }
    virtual sptr<FunctionDefinition> ToFunctionDefinition(){ return {}; }
    virtual sptr<InterfaceDefinition> ToInterfaceDefinition(){ return {}; }
    virtual sptr<ParameterDefinition> ToParameterDefinition(){ return {}; }
    virtual sptr<PropertyDefinition> ToPropertyDefinition(){ return {}; }
    virtual sptr<SectionDefinition> ToSectionDefinition(){ return {}; }
    virtual sptr<StructDefinition> ToStructDefinition(){ return {}; }
    virtual sptr<TemplateDefinition> ToTemplateDefinition(){ return {}; }
    virtual sptr<TemplateParameterDefinition> ToTemplateParameterDefinition(){ return {}; }
    virtual sptr<VariableDefinition> ToVariableDefinition(){ return {}; }
    
    virtual sptr<AssignExpression> ToAssignExpression(){ return {}; }
    virtual sptr<AsExpression> ToAsExpression(){ return {}; }
    virtual sptr<AwaitExpression> ToAwaitExpression(){ return {}; }
    virtual sptr<BinaryExpression> ToBinaryExpression(){ return {}; }
    virtual sptr<BooleanLiteralExpression> ToBooleanLiteralExpression(){ return {}; }
    virtual sptr<CachedExpression> ToCachedExpression(){ return {}; }
    virtual sptr<CallExpression> ToCallExpression(){ return {}; }
    virtual sptr<ConvertExpression> ToConvertExpression(){ return {}; }
    virtual sptr<DefaultValueExpression> ToDefaultValueExpression(){ return {}; }
    virtual sptr<EmitExpression> ToEmitExpression(){ return {}; }
    virtual sptr<Expression> ToExpression(){ return {}; }
    virtual sptr<FoldExpression> ToFoldExpression(){ return {}; }
    virtual sptr<IdentifierExpression> ToIdentifierExpression(){ return {}; }
    virtual sptr<IndexExpression> ToIndexExpression(){ return {}; }
    virtual sptr<IntegerLiteralExpression> ToIntegerLiteralExpression(){ return {}; }
    virtual sptr<IsExpression> ToIsExpression(){ return {}; }
    virtual sptr<NewExpression> ToNewExpression(){ return {}; }
    virtual sptr<NullLiteralExpression> ToNullLiteralExpression(){ return {}; }
    virtual sptr<NumberLiteralExpression> ToNumberLiteralExpression(){ return {}; }
    virtual sptr<PostfixExpression> ToPostfixExpression(){ return {}; }
    virtual sptr<PrefixExpression> ToPrefixExpression(){ return {}; }
    virtual sptr<SizeOfExpression> ToSizeOfExpression(){ return {}; }
    virtual sptr<StringLiteralExpression> ToStringLiteralExpression(){ return {}; }
    virtual sptr<TernaryExpression> ToTernaryExpression(){ return {}; }
    virtual sptr<TypeLiteralExpression> ToTypeLiteralExpression(){ return {}; }
    virtual sptr<TypeOfExpression> ToTypeOfExpression(){ return {}; }

    virtual sptr<TypeSpecifier> ToTypeSpecifier(){ return {}; }
    
    virtual sptr<AssertStatement> ToAssertStatement(){ return {}; }
    virtual sptr<BlockStatement> ToBlockStatement(){ return {}; }
    virtual sptr<ExposeStatement> ToExposeStatement(){ return {}; }
    virtual sptr<ExpressionStatement> ToExpressionStatement(){ return {}; }
    virtual sptr<ForStatement> ToForStatement(){ return {}; }
    virtual sptr<GotoStatement> ToGotoStatement(){ return {}; }
    virtual sptr<IfStatement> ToIfStatement(){ return {}; }
    virtual sptr<ReturnStatement> ToReturnStatement(){ return {}; }
    virtual sptr<Statement> ToStatement(){ return {}; }
    virtual sptr<VariableDefinitionStatement> ToVariableDefinitionStatement(){ return {}; }
    virtual sptr<WhileStatement> ToWhileStatement(){ return {}; }
};

} // fraze
