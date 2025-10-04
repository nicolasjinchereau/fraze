/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/ast/AST.h>
#include <fraze/ast/ASTVisitor.h>

namespace fraze {

void ASTVisitor::VisitChildNode(const sptr<ASTNode>& node)
{
    if(node) node->Accept(*this);
}


void ASTVisitor::Visit(const sptr<ASTRoot>& node)
{
    VisitChild(node->global);
}

void ASTVisitor::Visit(const sptr<BasicTypeDefinition>& node)
{
}

void ASTVisitor::Visit(const sptr<ClassDefinition>& node)
{
    for (auto& itr : node->interfaces)
        VisitChild(itr);

    for (auto& def : node->scope->definitions)
        VisitChild(def);
}

void ASTVisitor::Visit(const sptr<EnumDefinition>& node)
{
    for(auto& def : node->scope->definitions)
        VisitChild(def);
}

void ASTVisitor::Visit(const sptr<EnumMemberDefinition>& node)
{
    VisitChild(node->value);
}

void ASTVisitor::Visit(const sptr<FunctionDefinition>& node)
{
    VisitChild(node->returnType);

    for (auto& def : node->scope->definitions)
    {
        if(def->ToParameterDefinition())
            VisitChild(def);
    }

    for (auto& def : node->scope->definitions)
    {
        if(!def->ToParameterDefinition())
            VisitChild(def);
    }

    VisitChild(node->body);
}

void ASTVisitor::Visit(const sptr<InterfaceDefinition>& node)
{
    for (auto& def : node->scope->definitions)
        VisitChild(def);
}

void ASTVisitor::Visit(const sptr<ParameterDefinition>& node)
{
    VisitChild(node->typeSpec);
}

void ASTVisitor::Visit(const sptr<PropertyDefinition>& node)
{
    VisitChild(node->typeSpec);
    VisitChild(node->initializer);
}

void ASTVisitor::Visit(const sptr<SectionDefinition>& node)
{
    for(auto stmt : node->statements)
        VisitChild(stmt);

    for(size_t i = 0; i != node->scope->definitions.size(); ++i)
    {
        auto def = node->scope->definitions[i];
        if(!def->ToVariableDefinition())
            VisitChild(def);
    }
}

void ASTVisitor::Visit(const sptr<StructDefinition>& node)
{
    for(auto def : node->scope->definitions)
        VisitChild(def);
}

void ASTVisitor::Visit(const sptr<TemplateDefinition>& node)
{
}

void ASTVisitor::Visit(const sptr<TemplateParameterDefinition>& node)
{
    VisitChild(node->typeSpec);
}

void ASTVisitor::Visit(const sptr<VariableDefinition>& node)
{
    VisitChild(node->typeSpec);
    VisitChild(node->initializer);
}

void ASTVisitor::Visit(const sptr<AsExpression>& node)
{
    VisitChild(node->value);
    VisitChild(node->typeSpec);
}

void ASTVisitor::Visit(const sptr<AssignExpression>& node)
{
    VisitChild(node->left);
    VisitChild(node->right);
}

void ASTVisitor::Visit(const sptr<AwaitExpression>& node)
{
    VisitChild(node->expression);
    VisitChild(node->context);
}

void ASTVisitor::Visit(const sptr<BinaryExpression>& node)
{
    VisitChild(node->left);
    VisitChild(node->right);
}

void ASTVisitor::Visit(const sptr<BooleanLiteralExpression>& node)
{
}

void ASTVisitor::Visit(const sptr<CachedExpression>& node)
{
    VisitChild(node->cache);
    VisitChild(node->value);
}

void ASTVisitor::Visit(const sptr<CallExpression>& node)
{
    for(auto& arg : node->arguments)
        VisitChild(arg);

    VisitChild(node->target);
}

void ASTVisitor::Visit(const sptr<ConvertExpression>& node)
{
    VisitChild(node->resultTypeSpec);
    VisitChild(node->value);
}

void ASTVisitor::Visit(const sptr<DefaultValueExpression>& node)
{
    VisitChild(node->coroutineFieldInit);
    VisitChild(node->typeSpec);
}

void ASTVisitor::Visit(const sptr<EmitExpression>& node)
{
    VisitChild(node->expectedType);
    VisitChild(node->context);
}

void ASTVisitor::Visit(const sptr<FoldExpression>& node)
{
    VisitChild(node->body);
}

void ASTVisitor::Visit(const sptr<IdentifierExpression>& node)
{
    VisitChild(node->context);
}

void ASTVisitor::Visit(const sptr<IndexExpression>& node)
{
    VisitChild(node->target);
    VisitChild(node->arg);
}

void ASTVisitor::Visit(const sptr<IntegerLiteralExpression>& node)
{
}

void ASTVisitor::Visit(const sptr<IsExpression>& node)
{
    VisitChild(node->value);
    VisitChild(node->typeSpec);
}

void ASTVisitor::Visit(const sptr<NewExpression>& node)
{
    VisitChild(node->typeSpec);

    for(auto& arg : node->arguments)
        VisitChild(arg);

    VisitChild(node->argumentExpression);
}

void ASTVisitor::Visit(const sptr<NullLiteralExpression>& node)
{
}

void ASTVisitor::Visit(const sptr<NumberLiteralExpression>& node)
{
}

void ASTVisitor::Visit(const sptr<PostfixExpression>& node)
{
    VisitChild(node->arg);
}

void ASTVisitor::Visit(const sptr<PrefixExpression>& node)
{
    VisitChild(node->arg);
}

void ASTVisitor::Visit(const sptr<SizeOfExpression>& node)
{
    VisitChild(node->typeSpec);
}

void ASTVisitor::Visit(const sptr<StringLiteralExpression>& node)
{
}

void ASTVisitor::Visit(const sptr<TernaryExpression>& node)
{
    VisitChild(node->condition);
    VisitChild(node->trueValue);
    VisitChild(node->falseValue);
}

void ASTVisitor::Visit(const sptr<TypeSpecifier>& node)
{
    for(auto& arg : node->templateArgs)
        VisitChild(arg);
}

void ASTVisitor::Visit(const sptr<AssertStatement>& node)
{
    VisitChild(node->condition);
    VisitChild(node->message);
}

void ASTVisitor::Visit(const sptr<BlockStatement>& node)
{
    for(auto& stmt : node->statements)
        VisitChild(stmt);
}

void ASTVisitor::Visit(const sptr<ExposeStatement>& node)
{
    VisitChild(node->section);
}

void ASTVisitor::Visit(const sptr<ExpressionStatement>& node)
{
    VisitChild(node->expression);
}

void ASTVisitor::Visit(const sptr<ForStatement>& node)
{
    VisitChild(node->init);
    VisitChild(node->condition);
    VisitChild(node->iterate);
    VisitChild(node->body);
}

void ASTVisitor::Visit(const sptr<GotoStatement>& node)
{
    VisitChild(node->expression);
}

void ASTVisitor::Visit(const sptr<IfStatement>& node)
{
    VisitChild(node->condition);
    VisitChild(node->trueBranch);
    VisitChild(node->falseBranch);
}

void ASTVisitor::Visit(const sptr<ReturnStatement>& node)
{
    VisitChild(node->expression);
    VisitChild(node->context);
}

void ASTVisitor::Visit(const sptr<VariableDefinitionStatement>& node)
{
    VisitChild(node->variableDefinition);
}

void ASTVisitor::Visit(const sptr<WhileStatement>& node)
{
    VisitChild(node->condition);
    VisitChild(node->body);
}

} // fraze
