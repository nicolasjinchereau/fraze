/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <sstream>
#include <utf8.h>
#include <fraze/ast/AST.h>
#include <fraze/ast/ASTPrinter.h>

namespace fraze {

std::string ASTPrinter::GetPreamble(const SourceLocation& loc) const
{
    return std::format("{:>6}{}", loc.line, std::string(indent * tabWidth, ' '));
}

void ASTPrinter::VisitChildNode(const sptr<ASTNode>& node)
{
    ++indent;
    ASTVisitor::VisitChildNode(node);
    --indent;
}

/*****************************
*            ROOT            *
*****************************/

void ASTPrinter::Visit(const sptr<ASTRoot>& node)
{
    stream << GetPreamble(node->loc) << "ASTRoot" << std::endl;
    ASTVisitor::Visit(node);
}

/*****************************
*         DEFINITIONS        *
*****************************/

void ASTPrinter::Visit(const sptr<BasicTypeDefinition>& node)
{
    stream << GetPreamble(node->loc) << "BasicTypeDefinition " << node->name << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<ClassDefinition>& node)
{
    stream << GetPreamble(node->loc) << "ClassDefinition " << node->name << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<EnumDefinition>& node)
{
    stream << GetPreamble(node->loc) << "EnumDefinition " << node->name << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<EnumMemberDefinition>& node)
{
    stream << GetPreamble(node->loc) << "EnumMemberDefinition " << node->name << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<FunctionDefinition>& node)
{
    stream << GetPreamble(node->loc) << "FunctionDefinition " << node->name << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<InterfaceDefinition>& node)
{
    stream << GetPreamble(node->loc) << "InterfaceDefinition " << node->name << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<ParameterDefinition>& node)
{
    stream << GetPreamble(node->loc) << "ParameterDefinition " << node->name << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<PropertyDefinition>& node)
{
    stream << GetPreamble(node->loc) << "PropertyDefinition " << node->name << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<SectionDefinition>& node)
{
    stream << GetPreamble(node->loc) << "SectionDefinition " << node->name << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<StructDefinition>& node)
{
    stream << GetPreamble(node->loc) << "StructDefinition " << node->name << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<TemplateDefinition>& node)
{
    stream << GetPreamble(node->loc) << "TemplateDefinition " << node->name << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<TemplateParameterDefinition>& node)
{
    stream << GetPreamble(node->loc) << "TemplateParameterDefinition " << node->name << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<VariableDefinition>& node)
{
    stream << GetPreamble(node->loc) << "VariableDefinition " << node->name << std::endl;
    ASTVisitor::Visit(node);
}

/*****************************
*         EXPRESSIONS        *
*****************************/

void ASTPrinter::Visit(const sptr<AsExpression>& node)
{
    stream << GetPreamble(node->loc) << "AsExpression" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<AssignExpression>& node)
{
    stream << GetPreamble(node->loc) << "AssignExpression " << Lexer::GetTokenName(node->operation) << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<AwaitExpression>& node)
{
    stream << GetPreamble(node->loc) << "AwaitExpression" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<BinaryExpression>& node)
{
    stream << GetPreamble(node->loc) << "BinaryExpression " << Lexer::GetTokenName(node->operation) << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<BooleanLiteralExpression>& node)
{
    stream << GetPreamble(node->loc) << "BooleanLiteralExpression " << std::boolalpha << node->value << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<CachedExpression>& node)
{
    stream << GetPreamble(node->loc) << "CachedExpression" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<CallExpression>& node)
{
    stream << GetPreamble(node->loc) << "CallExpression" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<ConvertExpression>& node)
{
    stream << GetPreamble(node->loc) << "ConvertExpression" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<DefaultValueExpression>& node)
{
    stream << GetPreamble(node->loc) << "DefaultValueExpression" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<EmitExpression>& node)
{
    stream << GetPreamble(node->loc) << "EmitExpression ";

    size_t i = 0;
    for(auto& emission : node->emissions) {
        if(i++ > 0) std::cout << ", ";
        stream << "{ " << OpCodeNames[emission.op.code] << ", " << emission.op.arg1_u64 << " }";
    }

    stream << std::endl;

    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<FoldExpression>& node)
{
    stream << GetPreamble(node->loc) << "FoldExpression" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<IdentifierExpression>& node)
{
    stream << GetPreamble(node->loc) << "IdentifierExpression " << node->value << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<IndexExpression>& node)
{
    stream << GetPreamble(node->loc) << "IndexExpression" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<IntegerLiteralExpression>& node)
{
    stream << GetPreamble(node->loc) << "IntegerLiteral " << node->value << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<IsExpression>& node)
{
    stream << GetPreamble(node->loc) << "IsExpression" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<NewExpression>& node)
{
    stream << GetPreamble(node->loc) << "NewExpression" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<NullLiteralExpression>& node)
{
    stream << GetPreamble(node->loc) << "NullLiteralExpression" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<NumberLiteralExpression>& node)
{
    stream << GetPreamble(node->loc) << "NumberLiteral " << node->value << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<PostfixExpression>& node)
{
    stream << GetPreamble(node->loc) << "PostfixExpression" << Lexer::GetTokenName(node->operation) << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<PrefixExpression>& node)
{
    stream << GetPreamble(node->loc) << "PrefixExpression " << Lexer::GetTokenName(node->operation) << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<SizeOfExpression>& node)
{
    stream << GetPreamble(node->loc) << "SizeOfExpression " << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<StringLiteralExpression>& node)
{
    stream << GetPreamble(node->loc) << "StringLiteral \"" << node->value << "\"" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<TernaryExpression>& node)
{
    stream << GetPreamble(node->loc) << "TernaryExpression" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<TypeLiteralExpression>& node)
{
    stream << GetPreamble(node->loc) << "TypeLiteralExpression \"" << node->value << "\"" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<TypeOfExpression>& node)
{
    stream << GetPreamble(node->loc) << "TypeOfExpression" << std::endl;
    ASTVisitor::Visit(node);
}

/****************************
*         SPECIFIERS        *
****************************/

void ASTPrinter::Visit(const sptr<TypeSpecifier>& node)
{
    stream << GetPreamble(node->loc) << "TypeSpecifier " << node->GetTypeName() << std::endl;
    ASTVisitor::Visit(node);
}

/****************************
*         STATEMENTS        *
****************************/

void ASTPrinter::Visit(const sptr<AssertStatement>& node)
{
    stream << GetPreamble(node->loc) << "AssertStatement" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<BlockStatement>& node)
{
    stream << GetPreamble(node->loc) << "BlockStatement" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<ExposeStatement>& node)
{
    stream << GetPreamble(node->loc) << "ExposeStatement" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<ExpressionStatement>& node)
{
    stream << GetPreamble(node->loc) << "ExpressionStatement" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<ForStatement>& node)
{
    stream << GetPreamble(node->loc) << "ForStatement" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<GotoStatement>& node)
{
    stream << GetPreamble(node->loc) << "GotoStatement" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<IfStatement>& node)
{
    stream << GetPreamble(node->loc) << "IfStatement" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<ReturnStatement>& node)
{
    stream << GetPreamble(node->loc) << "ReturnStatement" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<VariableDefinitionStatement>& node)
{
    stream << GetPreamble(node->loc) << "VariableDefinitionStatement" << std::endl;
    ASTVisitor::Visit(node);
}

void ASTPrinter::Visit(const sptr<WhileStatement>& node)
{
    stream << GetPreamble(node->loc) << "WhileStatement" << std::endl;
    ASTVisitor::Visit(node);
}

} // fraze
