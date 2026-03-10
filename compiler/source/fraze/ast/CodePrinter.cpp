/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <sstream>
#include <utf8.h>
#include <fraze/ast/AST.h>
#include <fraze/ast/CodePrinter.h>

namespace fraze {

std::string CodePrinter::GetIndent() const
{
    return std::format("{}", std::string(indent * tabWidth, ' '));
}

/*****************************
*            ROOT            *
*****************************/

void CodePrinter::Visit(const sptr<ASTRoot>& node)
{
    ASTVisitor::Visit(node);
}

/*****************************
*         DEFINITIONS        *
*****************************/

void CodePrinter::Visit(const sptr<BasicTypeDefinition>& node)
{
    //stream << node->name << std::endl;
    stream << GetIndent() << "__basic_type " << node->name << ";\n";
    ASTVisitor::Visit(node);
}

void CodePrinter::Visit(const sptr<ClassDefinition>& node)
{
    stream << GetIndent() << "class " << node->name;

    if(!node->interfaces.empty())
    {
        int i = 0;
        stream << " : ";
        for(auto& itf : node->interfaces)
        {
            if(i++ > 0) stream << ", ";
            stream << itf->GetTypeName(true);
        }
    }
    
    stream << "\n";
    stream << GetIndent() << "{\n";
    ++indent;

    for (auto& def : node->scope->definitions)
    {
        if(auto varDef = def->ToClassDefinition())
            VisitChild(def);
    }

    for (auto& def : node->scope->definitions)
    {
        if(auto varDef = def->ToVariableDefinition())
            VisitChild(def);
    }

    for (auto& def : node->scope->definitions)
    {
        if(auto varDef = def->ToFunctionDefinition())
            VisitChild(def);
    }

    for (auto& def : node->scope->definitions)
    {
        if (!def->ToClassDefinition() &&
            !def->ToVariableDefinition() &&
            !def->ToFunctionDefinition())
        {
            VisitChild(def);
        }
    }

    --indent;
    stream << GetIndent() << "}\n";
}

void CodePrinter::Visit(const sptr<EnumDefinition>& node)
{
    stream << GetIndent() << "enum " << node->name << "\n";
    stream << GetIndent() << "{\n";
    ++indent;

    int i = 0;

    for(auto& def : node->scope->definitions)
    {
        VisitChild(def);

        ++i;

        if(i < node->scope->definitions.size())
            stream << ",\n";
    }

    --indent;
    stream << GetIndent() << "}\n";
}

void CodePrinter::Visit(const sptr<EnumMemberDefinition>& node)
{
    stream << GetIndent() << node->name;

    if(node->value)
    {
        stream << " = ";
        VisitChild(node->value);
    }
}

void CodePrinter::Visit(const sptr<FunctionDefinition>& node)
{
    stream << GetIndent();

    if(node->isExternal)
        stream << "extern ";

    if(node->isMember && node->isStatic)
        stream << "static ";

    stream << node->returnType->GetTypeName(true) << " " << node->name << "(";
    
    int i = 0;
    for(const auto& param : node->GetChildren<ParameterDefinition>())
    {
        if(i++ > 0) stream << ", ";
        stream << param->typeSpec->GetTypeName(true) << " " << param->name;
    }

    if(node->body)
    {
        stream << ")\n";
        VisitChild(node->body);
    }
    else
    {
        stream << ");\n";
    }
}

void CodePrinter::Visit(const sptr<InterfaceDefinition>& node)
{
    stream << GetIndent() << "interface " << node->name << "\n";
    stream << GetIndent() << "{\n";
    ++indent;

    ASTVisitor::Visit(node);

    --indent;
    stream << GetIndent() << "}\n";
}

void CodePrinter::Visit(const sptr<ParameterDefinition>& node)
{
    ASTVisitor::Visit(node);
}

void CodePrinter::Visit(const sptr<PropertyDefinition>& node)
{
    stream << GetIndent() << node->typeSpec->GetTypeName(true) << " " << node->name;

    if(node->initializer)
    {
        stream << " = ";
        VisitChild(node->initializer);
    }

    stream << ";\n";
}

void CodePrinter::Visit(const sptr<SectionDefinition>& node)
{
    stream << GetIndent() << "section " << node->name << "\n";
    stream << GetIndent() << "{\n";
    ++indent;

    for (auto& def : node->scope->definitions)
    {
        if(auto varDef = def->ToBasicTypeDefinition())
            VisitChild(def);
    }

    for (auto& def : node->scope->definitions)
    {
        if(auto varDef = def->ToInterfaceDefinition())
            VisitChild(def);
    }

    for (auto& def : node->scope->definitions)
    {
        if(auto varDef = def->ToClassDefinition())
            VisitChild(def);
    }

    for (auto& def : node->scope->definitions)
    {
        if(auto varDef = def->ToVariableDefinition())
            VisitChild(def);
    }

    for (auto& def : node->scope->definitions)
    {
        if(auto varDef = def->ToFunctionDefinition())
            VisitChild(def);
    }

    for (auto& def : node->scope->definitions)
    {
        if(auto varDef = def->ToSectionDefinition())
            VisitChild(def);
    }

    for (auto& def : node->scope->definitions)
    {
        if (!def->ToBasicTypeDefinition() &&
            !def->ToInterfaceDefinition() &&
            !def->ToClassDefinition() &&
            !def->ToVariableDefinition() &&
            !def->ToFunctionDefinition() &&
            !def->ToSectionDefinition())
        {
            VisitChild(def);
        }
    }

    --indent;
    stream << GetIndent() << "}\n";
}

void CodePrinter::Visit(const sptr<StructDefinition>& node)
{
    stream << GetIndent() << "struct " << node->name;

    stream << GetIndent() << "{\n";
    ++indent;

    for (auto& def : node->scope->definitions)
    {
        if(auto varDef = def->ToClassDefinition())
            VisitChild(def);
    }

    for (auto& def : node->scope->definitions)
    {
        if(auto varDef = def->ToVariableDefinition())
            VisitChild(def);
    }

    for (auto& def : node->scope->definitions)
    {
        if(auto varDef = def->ToFunctionDefinition())
            VisitChild(def);
    }

    for (auto& def : node->scope->definitions)
    {
        if (!def->ToClassDefinition() &&
            !def->ToVariableDefinition() &&
            !def->ToFunctionDefinition())
        {
            VisitChild(def);
        }
    }

    --indent;
    stream << GetIndent() << "}\n";
}

void CodePrinter::Visit(const sptr<TemplateDefinition>& node)
{
    ASTVisitor::Visit(node);
}

void CodePrinter::Visit(const sptr<TemplateParameterDefinition>& node)
{
    ASTVisitor::Visit(node);
}

void CodePrinter::Visit(const sptr<VariableDefinition>& node)
{
    stream << GetIndent() << node->typeSpec->GetTypeName(true) << " " << node->name;
    
    if(node->initializer)
    {
        stream << " = ";
        VisitChild(node->initializer);
    }

    stream << ";\n";
}

/*****************************
*         EXPRESSIONS        *
*****************************/

void CodePrinter::Visit(const sptr<AsExpression>& node)
{
    VisitChild(node->value);
    stream << " as ";
    VisitChild(node->typeSpec);
}

void CodePrinter::Visit(const sptr<AssignExpression>& node)
{
    VisitChild(node->left);
    stream << " " << TokenNames.at(node->operation) << " ";
    VisitChild(node->right);
}

void CodePrinter::Visit(const sptr<AwaitExpression>& node)
{
    stream << "await ";
    VisitChild(node->expression);
}

void CodePrinter::Visit(const sptr<BinaryExpression>& node)
{
    VisitChild(node->left);
    stream << " " << TokenNames.at(node->operation) << " ";
    VisitChild(node->right);
}

void CodePrinter::Visit(const sptr<BooleanLiteralExpression>& node)
{
    stream << (node->value ? "true" : "false");
}

void CodePrinter::Visit(const sptr<CachedExpression>& node)
{
    ASTVisitor::Visit(node);
}

void CodePrinter::Visit(const sptr<CallExpression>& node)
{
    VisitChild(node->target);
    stream << "(";

    int i = 0;
    for(auto& arg : node->arguments)
    {
        if(i++ > 0) stream << ", ";
        VisitChild(arg);
    }

    stream << ")";
}

void CodePrinter::Visit(const sptr<ConvertExpression>& node)
{
    stream << "convert<" << node->resultTypeSpec->GetTypeName(true) << ">(";
    VisitChild(node->value);
    stream << ")";
}

void CodePrinter::Visit(const sptr<DefaultValueExpression>& node)
{
    stream << "default(" << node->typeSpec->GetTypeName(true) << ")";
}

void CodePrinter::Visit(const sptr<EmitExpression>& node)
{
    ASTVisitor::Visit(node);
}

void CodePrinter::Visit(const sptr<FoldExpression>& node)
{
    ASTVisitor::Visit(node);
}

void CodePrinter::Visit(const sptr<IdentifierExpression>& node)
{
    if(node->context)
    {
        VisitChild(node->context);
        stream << ".";
    }

    stream << node->value;
}

void CodePrinter::Visit(const sptr<IndexExpression>& node)
{
    VisitChild(node->target);
    stream << "[";
    VisitChild(node->arg);
    stream << "]";
}

void CodePrinter::Visit(const sptr<IntegerLiteralExpression>& node)
{
    stream << node->value;
}

void CodePrinter::Visit(const sptr<IsExpression>& node)
{
    VisitChild(node->value);
    stream << " is ";
    VisitChild(node->typeSpec);
}

void CodePrinter::Visit(const sptr<NewExpression>& node)
{
    stream << "new " << node->typeSpec->GetTypeName(true) << "(";

    if(node->argumentExpression)
    {
        stream << "\n" << GetIndent() << "[{\n";
        ++indent;
        stream << GetIndent();
        VisitChild(node->argumentExpression);
        --indent;
        stream << "\n";
        stream << GetIndent() << "}]";
    }
    else
    {
        int i = 0;
        for(auto& arg : node->arguments)
        {
            if(i++ > 0) stream << ", ";
            VisitChild(arg);
        }
    }

    stream << ")";
}

void CodePrinter::Visit(const sptr<NullLiteralExpression>& node)
{
    stream << "null";
}

void CodePrinter::Visit(const sptr<NumberLiteralExpression>& node)
{
    stream << node->value;
}

void CodePrinter::Visit(const sptr<PostfixExpression>& node)
{
    VisitChild(node->arg);
    stream << TokenNames.at(node->operation);
}

void CodePrinter::Visit(const sptr<PrefixExpression>& node)
{
    stream << TokenNames.at(node->operation);
    VisitChild(node->arg);
}

void CodePrinter::Visit(const sptr<SizeOfExpression>& node)
{
    stream << "sizeof(" << node->typeSpec->GetTypeName() << ")";
}

void CodePrinter::Visit(const sptr<StringLiteralExpression>& node)
{
    stream << "\"" << node->value << "\"";
}

void CodePrinter::Visit(const sptr<TernaryExpression>& node)
{
    VisitChild(node->condition);
    stream << " ? ";
    VisitChild(node->trueValue);
    stream << " : ";
    VisitChild(node->falseValue);
}

void CodePrinter::Visit(const sptr<TypeLiteralExpression>& node)
{
    stream << "type(" << node->value << ")";
}

void CodePrinter::Visit(const sptr<TypeOfExpression>& node)
{
    stream << "typeof(" << node->typeSpec->GetTypeName() << ")";
}

/****************************
*         SPECIFIERS        *
****************************/

void CodePrinter::Visit(const sptr<TypeSpecifier>& node)
{
    stream << node->GetTypeName(true);
}

/****************************
*         STATEMENTS        *
****************************/

void CodePrinter::Visit(const sptr<AssertStatement>& node)
{
    stream << GetIndent() << "assert(" << std::endl;
    VisitChild(node->condition);

    if(node->message)
    {
        stream << ", ";
        VisitChild(node->message);
    }

    stream << ");\n";
}

void CodePrinter::Visit(const sptr<BlockStatement>& node)
{
    stream << GetIndent() << "{\n";
    ++indent;

    for(auto& stmt : node->statements)
    {
        stream << GetIndent();
        VisitChild(stmt);
    }

    --indent;
    stream << GetIndent() << "}\n";
}

void CodePrinter::Visit(const sptr<ExposeStatement>& node)
{
    stream << GetIndent() << "expose " << node->section->GetTypeName(true) << ";\n";
}

void CodePrinter::Visit(const sptr<ExpressionStatement>& node)
{
    VisitChild(node->expression);
    stream << ";\n";
}

void CodePrinter::Visit(const sptr<ForStatement>& node)
{
    stream << "for(";
    if(node->init)
        VisitChild(node->init);
    else
        stream << " ";
    
    stream << "; ";

    VisitChild(node->condition);
    stream << "; ";

    VisitChild(node->init);

    stream << ")\n";

    if(node->body->ToBlockStatement())
    {
        VisitChild(node->body);
    }
    else
    {
        ++indent;
        VisitChild(node->body);
        --indent;
    }
}

void CodePrinter::Visit(const sptr<GotoStatement>& node)
{
    stream << GetIndent() << "goto ";
    VisitChild(node->expression);
    stream << ";\n";
}

void CodePrinter::Visit(const sptr<IfStatement>& node)
{
    stream << "if(";
    VisitChild(node->condition);
    stream << ")\n";

    if(node->trueBranch->ToBlockStatement())
    {
        VisitChild(node->trueBranch);
    }
    else
    {
        ++indent;
        stream << GetIndent();
        VisitChild(node->trueBranch);
        --indent;
    }

    if(node->falseBranch)
    {
        stream << GetIndent() << "else\n";

        if(node->falseBranch->ToBlockStatement())
        {
            VisitChild(node->falseBranch);
        }
        else
        {
            ++indent;
            stream << GetIndent();
            VisitChild(node->falseBranch);
            --indent;
        }
    }
}

void CodePrinter::Visit(const sptr<ReturnStatement>& node)
{
    stream << "return";
    if(node->expression)
    {
        stream << " ";
        VisitChild(node->expression);
    }
    stream << ";\n";
}

void CodePrinter::Visit(const sptr<VariableDefinitionStatement>& node)
{
    auto varDef = node->variableDefinition;
    stream << varDef->typeSpec->GetTypeName(true) << " " << varDef->name;

    if(varDef->initializer)
    {
        stream << " = ";
        VisitChild(varDef->initializer);
    }

    stream << ";\n";
}

void CodePrinter::Visit(const sptr<WhileStatement>& node)
{
    stream << GetIndent() << "while(";
    VisitChild(node->condition);
    stream << ")\n";

    if(node->body->ToBlockStatement())
    {
        VisitChild(node->body);
    }
    else
    {
        ++indent;
        VisitChild(node->body);
        --indent;
    }
}

} // fraze
