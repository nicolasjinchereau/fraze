/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <fraze/ast/ASTFwd.h>
#include <fraze/common/Pointers.h>
#include <fraze/common/Stack.h>
#include <optional>
#include <memory>

namespace fraze {

class ASTVisitor
{
public:
    // optionally replace a node after visiting
    std::optional<sptr<ASTNode>> replacement;

    virtual ~ASTVisitor(){}
    
    virtual void VisitChildNode(const sptr<ASTNode>& node);
    
    template<class T>
    void VisitChild(sptr<T>& node)
    {
        VisitChildNode(node);

        if(replacement.has_value())
        {
            auto newNode = replacement.value();
            auto newNodeTyped = std::dynamic_pointer_cast<T>(newNode);
            
            assert(newNodeTyped || !newNode);
            node = newNodeTyped;

            replacement.reset();
        }
    }

    virtual void Visit(const sptr<ASTRoot>& node);

    virtual void Visit(const sptr<BasicTypeDefinition>& node);
    virtual void Visit(const sptr<ClassDefinition>& node);
    virtual void Visit(const sptr<EnumDefinition>& node);
    virtual void Visit(const sptr<EnumMemberDefinition>& node);
    virtual void Visit(const sptr<FunctionDefinition>& node);
    virtual void Visit(const sptr<InterfaceDefinition>& node);
    virtual void Visit(const sptr<ParameterDefinition>& node);
    virtual void Visit(const sptr<PropertyDefinition>& node);
    virtual void Visit(const sptr<SectionDefinition>& node);
    virtual void Visit(const sptr<StructDefinition>& node);
    virtual void Visit(const sptr<TemplateDefinition>& node);
    virtual void Visit(const sptr<TemplateParameterDefinition>& node);
    virtual void Visit(const sptr<VariableDefinition>& node);

    virtual void Visit(const sptr<AsExpression>& node);
    virtual void Visit(const sptr<AssignExpression>& node);
    virtual void Visit(const sptr<AwaitExpression>& node);
    virtual void Visit(const sptr<BinaryExpression>& node);
    virtual void Visit(const sptr<BooleanLiteralExpression>& node);
    virtual void Visit(const sptr<CachedExpression>& node);
    virtual void Visit(const sptr<CastExpression>& node);
    virtual void Visit(const sptr<CallExpression>& node);
    virtual void Visit(const sptr<ConvertExpression>& node);
    virtual void Visit(const sptr<DefaultValueExpression>& node);
    virtual void Visit(const sptr<EmitExpression>& node);
    virtual void Visit(const sptr<FoldExpression>& node);
    virtual void Visit(const sptr<IdentifierExpression>& node);
    virtual void Visit(const sptr<IndexExpression>& node);
    virtual void Visit(const sptr<IntegerLiteralExpression>& node);
    virtual void Visit(const sptr<IsExpression>& node);
    virtual void Visit(const sptr<NewExpression>& node);
    virtual void Visit(const sptr<NullLiteralExpression>& node);
    virtual void Visit(const sptr<NumberLiteralExpression>& node);
    virtual void Visit(const sptr<PostfixExpression>& node);
    virtual void Visit(const sptr<PrefixExpression>& node);
    virtual void Visit(const sptr<SizeOfExpression>& node);
    virtual void Visit(const sptr<StringLiteralExpression>& node);
    virtual void Visit(const sptr<TernaryExpression>& node);
    virtual void Visit(const sptr<TypeLiteralExpression>& node);
    virtual void Visit(const sptr<TypeOfExpression>& node);

    virtual void Visit(const sptr<TypeSpecifier>& node);

    virtual void Visit(const sptr<AssertStatement>& node);
    virtual void Visit(const sptr<BlockStatement>& node);
    virtual void Visit(const sptr<ExposeStatement>& node);
    virtual void Visit(const sptr<ExpressionStatement>& node);
    virtual void Visit(const sptr<ForStatement>& node);
    virtual void Visit(const sptr<GotoStatement>& node);
    virtual void Visit(const sptr<IfStatement>& node);
    virtual void Visit(const sptr<ReturnStatement>& node);
    virtual void Visit(const sptr<VariableDefinitionStatement>& node);
    virtual void Visit(const sptr<WhileStatement>& node);
};

} // fraze
