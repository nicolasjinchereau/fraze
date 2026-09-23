/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <cmath>
#include <format>
#include <string>
#include <fraze/ast/AST.h>
#include <fraze/ast/CodePrinter.h>

namespace fraze {

/*****************************
*           HELPERS          *
*****************************/

void CodePrinter::PrintIndent()
{
    stream << std::string(indent * tabWidth, ' ');
}

// Prints a scope's definitions in declaration order, with a blank line around multi-line definitions.
void CodePrinter::PrintDefinitions(Scope* scope, bool skipVariables, bool hasPrecedingContent)
{
    bool hasPrevious = hasPrecedingContent;
    bool previousIsMultiLine = hasPrecedingContent;
    PropertyDefinition* property = nullptr; // the most recent property, whose accessors follow it

    // a section's definitions are split between the files they're located in
    bool isSectionScope = scope->owner->ToSectionDefinition() != nullptr;

    for(auto& def : scope->definitions)
    {
        // template parameters are printed in the header of their template
        if(def->ToTemplateParameterDefinition())
            continue;

        if(skipVariables && def->ToVariableDefinition())
            continue;

        if(isSectionScope && !HasContentInPrintedFile(def))
            continue;

        // a property and its accessors are printed as a group, without blank lines between them
        auto func = def->ToFunctionDefinition();
        bool isAccessor = property && func && (func->name == property->getterName || func->name == property->setterName);

        if(!isAccessor)
            property = def->ToPropertyDefinition().get();

        bool isMultiLine = IsMultiLine(def);

        if(hasPrevious && !isAccessor && (previousIsMultiLine || isMultiLine))
            stream << "\n";

        VisitChildNode(def);

        hasPrevious = true;
        previousIsMultiLine = isMultiLine;
    }
}

bool CodePrinter::IsInPrintedFile(const SourceLocation& loc) const
{
    return !printedFile || loc.file.view() == *printedFile;
}

// true if any part of 'def' is printed in the printed file, since a section's contents can come from several files
bool CodePrinter::HasContentInPrintedFile(const sptr<Definition>& def) const
{
    if(!printedFile)
        return true;

    auto section = def->ToSectionDefinition();
    if(!section)
        return IsInPrintedFile(def->loc);

    for(auto& stmt : section->statements)
    {
        if(IsInPrintedFile(stmt->loc))
            return true;
    }

    // a section's variables are printed with its statements
    for(auto& child : section->scope->definitions)
    {
        if(!child->ToVariableDefinition() && HasContentInPrintedFile(child))
            return true;
    }

    return false;
}

void CodePrinter::PrintTemplateParameters(const sptr<TemplateDefinition>& node)
{
    // an instance's name already includes its template arguments
    if(!node->IsTemplateDeclaration())
        return;

    stream << "<";

    size_t i = 0;
    for(const auto& param : node->GetChildren<TemplateParameterDefinition>())
    {
        if(i++ > 0) stream << ","; // no space, like TypeSpecifier::GetTemplateArgs, so names match when searched
        stream << param->name;
    }

    stream << ">";
}

// Prints state that has no keyword, named after its C++ field, e.g. '[isCoroutineState, originalClassType: App]'.
// A definition's attributes go on their own line above it; a parameter's go inline before it.
void CodePrinter::PrintAttributes(const std::vector<std::string>& attributes, bool ownLine)
{
    if(attributes.empty())
        return;

    if(ownLine)
        PrintIndent();

    stream << "[";

    size_t i = 0;
    for(auto& attribute : attributes)
    {
        if(i++ > 0) stream << ", ";
        stream << attribute;
    }

    stream << "]" << (ownLine ? "\n" : " ");
}

void CodePrinter::PrintVariable(const sptr<VariableDefinition>& node, bool qualifyName, bool printInitializer)
{
    if(node->isPrivate)
        stream << "private ";

    if(node->isStatic)
        stream << "static ";

    VisitChildNode(node->typeSpec);
    stream << " " << (qualifyName ? node->qualifiedName : node->name);

    if(printInitializer && node->initializer)
    {
        stream << " = ";
        PrintExpression(node->initializer, Precedence::Assignment);
    }
}

void CodePrinter::PrintBlock(const sptr<BlockStatement>& node, bool endLine)
{
    PrintIndent();
    stream << "{\n";
    ++indent;

    for(auto& stmt : node->statements)
        VisitChildNode(stmt);

    --indent;
    PrintIndent();
    stream << "}";

    if(endLine)
        stream << "\n";
}

// prints the body of an if, for, or while, indenting it unless it's a block
void CodePrinter::PrintBody(const sptr<Statement>& node)
{
    if(node->ToBlockStatement())
    {
        VisitChildNode(node);
    }
    else
    {
        ++indent;
        VisitChildNode(node);
        --indent;
    }
}

// prints an if statement from the current position, so 'else if' can share a line
void CodePrinter::PrintIfStatement(const sptr<IfStatement>& node)
{
    stream << "if(";
    PrintExpression(node->condition, Precedence::Assignment);
    stream << ")\n";

    PrintBody(node->trueBranch);

    if(node->falseBranch)
    {
        PrintIndent();

        if(auto elseIf = node->falseBranch->ToIfStatement())
        {
            stream << "else ";
            PrintIfStatement(elseIf);
        }
        else
        {
            stream << "else\n";
            PrintBody(node->falseBranch);
        }
    }
}

// prints a for statement's init or iterate statement without indentation or a semicolon
void CodePrinter::PrintInlineStatement(const sptr<Statement>& node)
{
    if(!node)
        return;

    if(auto varDefStmt = node->ToVariableDefinitionStatement())
        PrintVariable(varDefStmt->variableDefinition, false, true);
    else if(auto exprStmt = node->ToExpressionStatement())
        PrintExpression(exprStmt->expression, Precedence::Assignment);
}

// Prints 'expr', parenthesized if it binds looser than its position requires. The AST has no
// grouping nodes, so all parentheses in the output come from here.
void CodePrinter::PrintExpression(const sptr<Expression>& expr, Precedence minPrecedence)
{
    if(!expr)
        return;

    bool needsParens = GetPrecedence(expr) < minPrecedence;

    if(needsParens)
        stream << "(";

    VisitChildNode(expr);

    if(needsParens)
        stream << ")";
}

void CodePrinter::PrintArguments(const std::vector<sptr<Expression>>& args)
{
    size_t i = 0;
    for(auto& arg : args)
    {
        if(i++ > 0) stream << ", ";
        PrintExpression(arg, Precedence::Assignment);
    }
}

void CodePrinter::PrintTemplateArguments(const std::vector<sptr<TypeSpecifier>>& args)
{
    if(args.empty())
        return;

    stream << "<";

    size_t i = 0;
    for(auto& arg : args)
    {
        if(i++ > 0) stream << ","; // no space, like TypeSpecifier::GetTemplateArgs
        VisitChildNode(arg);
    }

    stream << ">";
}

void CodePrinter::PrintStringLiteral(std::string_view value)
{
    stream << '"';

    for(char c : value)
    {
        switch(c)
        {
        case '"':  stream << "\\\""; break;
        case '\\': stream << "\\\\"; break;
        case '\n': stream << "\\n"; break;
        case '\r': stream << "\\r"; break;
        case '\t': stream << "\\t"; break;
        case '\b': stream << "\\b"; break;
        case '\f': stream << "\\f"; break;
        default:
            if((unsigned char)c < 0x20)
                stream << std::format("\\u{:04x}", (unsigned char)c);
            else
                stream << c;
        }
    }

    stream << '"';
}

CodePrinter::Precedence CodePrinter::GetPrecedence(const sptr<Expression>& expr)
{
    if(auto binary = expr->ToBinaryExpression())
        return GetBinaryPrecedence(binary->operation);

    if(expr->ToAssignExpression())
        return Precedence::Assignment;

    if(expr->ToTernaryExpression())
        return Precedence::Ternary;

    if(expr->ToAwaitExpression())
        return Precedence::Await;

    if(expr->ToPrefixExpression() || StartsWithSign(expr))
        return Precedence::Prefix;

    if (expr->ToPostfixExpression() ||
        expr->ToAsExpression() ||
        expr->ToIsExpression() ||
        expr->ToCallExpression() ||
        expr->ToIndexExpression())
    {
        return Precedence::Postfix;
    }

    if(auto ident = expr->ToIdentifierExpression(); ident && ident->context)
        return Precedence::Postfix;

    return Precedence::Primary;
}

CodePrinter::Precedence CodePrinter::GetBinaryPrecedence(TokenType operation)
{
    switch(operation)
    {
    case TokenType::LogicalOr:
        return Precedence::LogicalOr;
    case TokenType::LogicalAnd:
        return Precedence::LogicalAnd;
    case TokenType::Equal:
    case TokenType::NotEqual:
        return Precedence::Equality;
    case TokenType::Less:
    case TokenType::LessEqual:
    case TokenType::Greater:
    case TokenType::GreaterEqual:
        return Precedence::Comparison;
    case TokenType::BitOr:
        return Precedence::BitOr;
    case TokenType::BitXor:
        return Precedence::BitXor;
    case TokenType::BitAnd:
    case TokenType::BitTest:
        return Precedence::BitAnd;
    case TokenType::LeftShift:
    case TokenType::RightShift:
        return Precedence::Shift;
    case TokenType::Add:
    case TokenType::Sub:
        return Precedence::AddSub;
    case TokenType::Mul:
    case TokenType::Div:
    case TokenType::Mod:
        return Precedence::MulDivMod;
    default:
        return Precedence::Assignment; // unknown, so always parenthesized
    }
}

bool CodePrinter::IsMultiLine(const sptr<Definition>& def)
{
    if(auto func = def->ToFunctionDefinition())
        return func->body != nullptr;

    return def->ToClassDefinition()
        || def->ToPropertyDefinition()
        || def->ToStructDefinition()
        || def->ToInterfaceDefinition()
        || def->ToEnumDefinition()
        || def->ToSectionDefinition();
}

// true if 'expr' prints starting with '+' or '-', which would merge with a preceding '+' or '-'
bool CodePrinter::StartsWithSign(const sptr<Expression>& expr)
{
    if(auto prefix = expr->ToPrefixExpression())
    {
        return prefix->operation == TokenType::Add
            || prefix->operation == TokenType::Sub
            || prefix->operation == TokenType::Increment
            || prefix->operation == TokenType::Decrement;
    }

    if(auto integer = expr->ToIntegerLiteralExpression())
        return integer->value < 0;

    if(auto number = expr->ToNumberLiteralExpression())
        return std::signbit(number->value);

    return false;
}

/*****************************
*            ROOT            *
*****************************/

void CodePrinter::Visit(const sptr<ASTRoot>& node)
{
    ASTVisitor::Visit(node);
    VisitChildNode(node->global);
}

/*****************************
*         DEFINITIONS        *
*****************************/

void CodePrinter::Visit(const sptr<BasicTypeDefinition>& node)
{
    PrintIndent();
    stream << "__basic_type " << node->name << ";\n";
}

void CodePrinter::Visit(const sptr<ClassDefinition>& node)
{
    std::vector<std::string> attributes;

    if(node->isFunctor)
        attributes.push_back("isFunctor");

    if(node->isCoroutineState)
        attributes.push_back("isCoroutineState");

    if(node->originalClassType)
        attributes.push_back(std::format("originalClassType: {}", node->originalClassType->GetTypeName(true).view()));

    PrintAttributes(attributes, true);
    PrintIndent();

    if(node->isExternal)
        stream << "extern ";

    stream << "class " << node->name;
    PrintTemplateParameters(node);

    if(!node->interfaces.empty())
    {
        stream << " : ";

        size_t i = 0;
        for(auto& itf : node->interfaces)
        {
            if(i++ > 0) stream << ", ";
            VisitChildNode(itf);
        }
    }

    stream << "\n";
    PrintIndent();
    stream << "{\n";
    ++indent;

    PrintDefinitions(node->scope.get(), false, false);

    --indent;
    PrintIndent();
    stream << "}\n";
}

void CodePrinter::Visit(const sptr<EnumDefinition>& node)
{
    PrintIndent();
    stream << "enum " << node->name << "\n";
    PrintIndent();
    stream << "{\n";
    ++indent;

    size_t i = 0;
    for(auto& def : node->scope->definitions)
    {
        VisitChildNode(def);

        if(++i != node->scope->definitions.size())
            stream << ",";

        stream << "\n";
    }

    --indent;
    PrintIndent();
    stream << "}\n";
}

void CodePrinter::Visit(const sptr<EnumMemberDefinition>& node)
{
    PrintIndent();
    stream << node->name;

    if(node->value)
    {
        stream << " = ";
        PrintExpression(node->value, Precedence::Assignment);
    }
}

void CodePrinter::Visit(const sptr<FunctionDefinition>& node)
{
    std::vector<std::string> attributes;

    if(node->isAbstract)
        attributes.push_back("isAbstract");

    if(node->isConstructor)
        attributes.push_back("isConstructor");

    if(node->isCoroutine)
        attributes.push_back("isCoroutine");

    if(node->externalIntrinsic)
        attributes.push_back("externalIntrinsic");

    PrintAttributes(attributes, true);
    PrintIndent();

    if(node->isPrivate)
        stream << "private ";

    if(node->isStatic)
        stream << "static ";

    if(node->isExternal)
        stream << "extern ";

    VisitChildNode(node->returnType);
    stream << " " << node->name;
    PrintTemplateParameters(node);
    stream << "(";

    size_t i = 0;
    for(const auto& param : node->GetChildren<ParameterDefinition>())
    {
        if(i++ > 0) stream << ", ";
        VisitChildNode(param);
    }

    stream << ")";

    if(node->body)
    {
        stream << "\n";
        PrintBlock(node->body, true);
    }
    else
    {
        stream << ";\n";
    }
}

void CodePrinter::Visit(const sptr<InterfaceDefinition>& node)
{
    if(node->isFunctor)
        PrintAttributes({ "isFunctor" }, true);

    PrintIndent();
    stream << "interface " << node->name;
    PrintTemplateParameters(node);
    stream << "\n";
    PrintIndent();
    stream << "{\n";
    ++indent;

    PrintDefinitions(node->scope.get(), false, false);

    --indent;
    PrintIndent();
    stream << "}\n";
}

void CodePrinter::Visit(const sptr<ParameterDefinition>& node)
{
    if(node->isReference)
        PrintAttributes({ "isReference" }, false);

    VisitChildNode(node->typeSpec);
    stream << " " << node->name;
}

void CodePrinter::Visit(const sptr<PropertyDefinition>& node)
{
    PrintIndent();

    if(node->isPrivate)
        stream << "private ";

    if(node->isStatic)
        stream << "static ";

    VisitChildNode(node->typeSpec);
    stream << " " << node->name << " { ";

    if(!node->getterName.empty())
        stream << "get: " << node->getterName << "; ";

    if(!node->setterName.empty())
        stream << "set: " << node->setterName << "; ";

    stream << "}";

    if(node->initializer)
    {
        stream << " = ";
        PrintExpression(node->initializer, Precedence::Assignment);
        stream << ";";
    }

    stream << "\n";
}

void CodePrinter::Visit(const sptr<SectionDefinition>& node)
{
    // the global section's contents are printed at the top level
    bool isGlobal = node->IsGlobal();

    if(!isGlobal)
    {
        PrintIndent();
        stream << "section " << node->name << "\n";
        PrintIndent();
        stream << "{\n";
        ++indent;
    }

    // The section's code runs before anything else in it, and its variables are defined by
    // statements in that code, so they're printed there instead of with the other definitions.
    bool hasStatements = false;

    for(auto& stmt : node->statements)
    {
        if(!IsInPrintedFile(stmt->loc))
            continue;

        VisitChildNode(stmt);
        hasStatements = true;
    }

    PrintDefinitions(node->scope.get(), true, hasStatements);

    if(!isGlobal)
    {
        --indent;
        PrintIndent();
        stream << "}\n";
    }
}

void CodePrinter::Visit(const sptr<StructDefinition>& node)
{
    PrintIndent();
    stream << "struct " << node->name;
    PrintTemplateParameters(node);
    stream << "\n";
    PrintIndent();
    stream << "{\n";
    ++indent;

    PrintDefinitions(node->scope.get(), false, false);

    --indent;
    PrintIndent();
    stream << "}\n";
}

void CodePrinter::Visit(const sptr<TemplateDefinition>& node)
{
}

void CodePrinter::Visit(const sptr<TemplateParameterDefinition>& node)
{
    stream << node->name;
}

void CodePrinter::Visit(const sptr<VariableDefinition>& node)
{
    // a static field's initializer runs in a statement in the global section,
    // except in a template declaration, which is never instantiated as-is
    bool printInitializer = !node->isStatic || node->IsPartOfTemplateDeclaration();

    PrintIndent();
    PrintVariable(node, false, printInitializer);
    stream << ";\n";
}

/*****************************
*         EXPRESSIONS        *
*****************************/

void CodePrinter::Visit(const sptr<ArrayCountExpression>& node)
{
    stream << "countof(";
    PrintExpression(node->array, Precedence::Assignment);
    stream << ")";
}

void CodePrinter::Visit(const sptr<AsExpression>& node)
{
    PrintExpression(node->value, Precedence::Postfix);
    stream << " as ";
    VisitChildNode(node->typeSpec);
}

void CodePrinter::Visit(const sptr<AssignExpression>& node)
{
    PrintExpression(node->left, Precedence::Ternary);
    stream << " " << TokenNames.at(node->operation) << " ";
    PrintExpression(node->right, Precedence::Assignment);
}

void CodePrinter::Visit(const sptr<AwaitExpression>& node)
{
    stream << "await ";
    PrintExpression(node->expression, Precedence::Await);
}

void CodePrinter::Visit(const sptr<BinaryExpression>& node)
{
    // binary operators are left-associative, so only the right operand needs to bind tighter
    Precedence precedence = GetBinaryPrecedence(node->operation);
    PrintExpression(node->left, precedence);
    stream << " " << TokenNames.at(node->operation) << " ";
    PrintExpression(node->right, Precedence(int(precedence) + 1));
}

void CodePrinter::Visit(const sptr<BooleanLiteralExpression>& node)
{
    stream << (node->value ? "true" : "false");
}

void CodePrinter::Visit(const sptr<CastExpression>& node)
{
    stream << "cast<";
    VisitChildNode(node->resultTypeSpec);
    stream << ">(";
    PrintExpression(node->value, Precedence::Assignment);
    stream << ")";
}

void CodePrinter::Visit(const sptr<CallExpression>& node)
{
    // a resolved function is named in full to show which overload or template instance was chosen
    auto ident = node->target->ToIdentifierExpression();
    auto func = ident && ident->targetDef ? ident->targetDef->ToFunctionDefinition() : nullptr;

    if(func)
        stream << func->qualifiedName;
    else
        PrintExpression(node->target, Precedence::Postfix);

    stream << "(";
    PrintArguments(node->arguments);
    stream << ")";
}

void CodePrinter::Visit(const sptr<CheckSiteExpression>& node)
{
    stream << "checksite(";
    PrintStringLiteral(node->message);
    stream << ")";
}

void CodePrinter::Visit(const sptr<ConvertExpression>& node)
{
    stream << "convert<";
    VisitChildNode(node->resultTypeSpec);
    stream << ">(";
    PrintExpression(node->value, Precedence::Assignment);
    stream << ")";
}

void CodePrinter::Visit(const sptr<DefaultValueExpression>& node)
{
    stream << "default(";

    // a coroutine field declared 'var' has no type until it's inferred during analysis
    if(node->typeSpec)
        VisitChildNode(node->typeSpec);
    else
        stream << "var";

    stream << ")";
}

void CodePrinter::Visit(const sptr<FoldExpression>& node)
{
    stream << "fold\n";
    PrintBlock(node->body, false);
}

void CodePrinter::Visit(const sptr<IdentifierExpression>& node)
{
    if(node->context)
    {
        PrintExpression(node->context, Precedence::Postfix);
        stream << ".";
    }

    stream << node->value;
    PrintTemplateArguments(node->templateArgs);
}

void CodePrinter::Visit(const sptr<IndexExpression>& node)
{
    PrintExpression(node->target, Precedence::Postfix);
    stream << "[";
    PrintExpression(node->arg, Precedence::Assignment);
    stream << "]";
}

void CodePrinter::Visit(const sptr<IntegerLiteralExpression>& node)
{
    stream << node->value;
}

void CodePrinter::Visit(const sptr<IsExpression>& node)
{
    PrintExpression(node->value, Precedence::Postfix);
    stream << " is ";
    VisitChildNode(node->typeSpec);
}

void CodePrinter::Visit(const sptr<NewExpression>& node)
{
    stream << "new";

    if(node->allocExpression)
    {
        stream << "(";
        PrintExpression(node->allocExpression, Precedence::Assignment);
        stream << ")";
    }

    stream << " ";

    if(node->argumentExpression)
    {
        // Parser counts the brackets around the length as one of the type's array dimensions
        stream << node->typeSpec->GetElementTypeName(true);

        for(int i = 1; i < node->typeSpec->arrayDimensions; ++i)
            stream << "[]";

        stream << "[";
        PrintExpression(node->argumentExpression, Precedence::Assignment);
        stream << "]";
    }
    else
    {
        VisitChildNode(node->typeSpec);
        stream << "{";
        PrintArguments(node->arguments);
        stream << "}";
    }
}

void CodePrinter::Visit(const sptr<NullLiteralExpression>& node)
{
    stream << "null";
}

void CodePrinter::Visit(const sptr<NumberLiteralExpression>& node)
{
    std::string text = std::format("{}", node->value);

    // without a decimal point or exponent, a num would read as an int
    if(text.find_first_of(".en") == std::string::npos)
        text += ".0";

    stream << text;
}

void CodePrinter::Visit(const sptr<PostfixExpression>& node)
{
    PrintExpression(node->arg, Precedence::Postfix);
    stream << TokenNames.at(node->operation);
}

void CodePrinter::Visit(const sptr<PrefixExpression>& node)
{
    stream << TokenNames.at(node->operation);

    // keeps '- -x' from printing as '--x'
    if(StartsWithSign(node) && StartsWithSign(node->arg))
        stream << " ";

    PrintExpression(node->arg, Precedence::Prefix);
}

void CodePrinter::Visit(const sptr<SizeOfExpression>& node)
{
    stream << "sizeof(";
    VisitChildNode(node->typeSpec);
    stream << ")";
}

void CodePrinter::Visit(const sptr<StringLiteralExpression>& node)
{
    PrintStringLiteral(node->value);
}

void CodePrinter::Visit(const sptr<TernaryExpression>& node)
{
    PrintExpression(node->condition, Precedence::LogicalOr);
    stream << " ? ";
    PrintExpression(node->trueValue, Precedence::Assignment);
    stream << " : ";
    PrintExpression(node->falseValue, Precedence::Ternary);
}

void CodePrinter::Visit(const sptr<TypeLiteralExpression>& node)
{
    stream << "typeid(";
    PrintStringLiteral(node->value);
    stream << ")";
}

void CodePrinter::Visit(const sptr<TypeOfExpression>& node)
{
    stream << "typeof(";
    VisitChildNode(node->typeSpec);
    stream << ")";
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
    PrintIndent();
    stream << "assert(";
    PrintExpression(node->condition, Precedence::Assignment);
    stream << ", ";
    PrintExpression(node->message, Precedence::Assignment);
    stream << ");\n";
}

void CodePrinter::Visit(const sptr<BlockStatement>& node)
{
    PrintBlock(node, true);
}

void CodePrinter::Visit(const sptr<EmptyStatement>& node)
{
    PrintIndent();
    stream << ";\n";
}

void CodePrinter::Visit(const sptr<ExposeStatement>& node)
{
    PrintIndent();
    stream << "expose ";
    VisitChildNode(node->section);
    stream << ";\n";
}

void CodePrinter::Visit(const sptr<ExpressionStatement>& node)
{
    PrintIndent();
    PrintExpression(node->expression, Precedence::Assignment);
    stream << ";\n";
}

void CodePrinter::Visit(const sptr<ForStatement>& node)
{
    PrintIndent();
    stream << "for(";
    PrintInlineStatement(node->init);
    stream << "; ";
    PrintExpression(node->condition, Precedence::Assignment);
    stream << "; ";
    PrintInlineStatement(node->iterate);
    stream << ")\n";

    PrintBody(node->body);
}

void CodePrinter::Visit(const sptr<GotoStatement>& node)
{
    PrintIndent();
    stream << "goto ";
    PrintExpression(node->expression, Precedence::Assignment);
    stream << ";\n";
}

void CodePrinter::Visit(const sptr<IfStatement>& node)
{
    PrintIndent();
    PrintIfStatement(node);
}

void CodePrinter::Visit(const sptr<ReturnStatement>& node)
{
    PrintIndent();
    stream << "return";

    if(node->expression)
    {
        stream << " ";
        PrintExpression(node->expression, Precedence::Assignment);
    }

    stream << ";\n";
}

void CodePrinter::Visit(const sptr<VariableDefinitionStatement>& node)
{
    // a static field is initialized by a statement in the global section, so it's named in full there
    auto& varDef = node->variableDefinition;
    bool isOwnedElsewhere = varDef->parent != node->enclosingScope->owner;

    PrintIndent();
    PrintVariable(varDef, isOwnedElsewhere, true);
    stream << ";\n";
}

void CodePrinter::Visit(const sptr<WhileStatement>& node)
{
    PrintIndent();
    stream << "while(";
    PrintExpression(node->condition, Precedence::Assignment);
    stream << ")\n";

    PrintBody(node->body);
}

} // fraze
