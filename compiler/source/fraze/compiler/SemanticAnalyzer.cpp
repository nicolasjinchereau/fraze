/*--------------------------------------------------------------*
*  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/common/Utility.h>
#include <fraze/compiler/Compiler.h>
#include <fraze/compiler/Parser.h>
#include <fraze/compiler/SemanticAnalyzer.h>
#include <fraze/ast/type/Type.h>
#include <fraze/ast/ASTPrinter.h>
#include <fraze/common/ScopeUtil.h>
#include <format>
#include <print>
#include <ranges>

namespace fraze {

SemanticAnalyzer::SemanticAnalyzer()
{
}

Type* SemanticAnalyzer::EvaluateTypeChecked(const sptr<Expression>& expr)
{
    Type* type = expr->EvaluateType();
    ENFORCE(!!type, expr->loc, "expression cannot be evaluated");
    return type;
}

bool SemanticAnalyzer::IsFunctionAssignable(const sptr<FunctionDefinition>& leftFunc, const sptr<FunctionDefinition>& rightFunc, bool skipImplicitThisParam)
{
    auto leftParams = leftFunc->GetChildren<ParameterDefinition>();
    auto rightParams = rightFunc->GetChildren<ParameterDefinition>();
    
    auto leftParam = leftParams.begin();
    auto rightParam = rightParams.begin();

    if(skipImplicitThisParam)
    {
        if(leftFunc->HasImplicitThisParam())
            ++leftParam;

        if(rightFunc->HasImplicitThisParam())
            ++rightParam;
    }

    size_t leftParamCount = std::distance(leftParam, leftParams.end());
    size_t rightParamCount = std::distance(rightParam, rightParams.end());

    if(leftParamCount != rightParamCount)
        return false;

    VisitChild(leftFunc->returnType);
    VisitChild(rightFunc->returnType);

    Type* leftReturnType = leftFunc->returnType->type;
    Type* rightReturnType = rightFunc->returnType->type;

    if(leftReturnType != rightReturnType)
        return false;

    for( ; leftParam != leftParams.end(); ++leftParam, ++rightParam)
    {
        auto& leftParamTypeSpec = (*leftParam)->typeSpec;
        auto& rightParamTypeSpec = (*rightParam)->typeSpec;

        VisitChild(leftParamTypeSpec);
        VisitChild(rightParamTypeSpec);

        auto leftParamType = leftParamTypeSpec->type;
        auto rightParamType = rightParamTypeSpec->type;

        if(leftParamType != rightParamType)
            return false;
    }

    return true;
}

bool SemanticAnalyzer::IsAssignable(Type* leftType, Type* rightType, TokenType operation, bool* needsConversion)
{
    // ensure types can be evaluated
    if(!leftType) return false;
    if(!rightType) return false;

    bool valid = false;
    bool convert = false;

    if(leftType->IsPlaceholder() || rightType->IsPlaceholder())
    {
        convert = false;
        valid = true;
    }
    else if(operation == TokenType::Assign)
    {
        if(leftType == rightType)
        {
            valid = true;
        }
        else if(leftType->IsFunctorInterface() && rightType->IsFunction())
        {
            auto leftInterface = leftType->GetDefinition()->ToInterfaceDefinition();
            sptr<FunctionDefinition> leftFunc = leftInterface->GetFunction("invoke");
            sptr<FunctionDefinition> rightFunc = rightType->GetDefinition()->ToFunctionDefinition();

            if(IsFunctionAssignable(leftFunc, rightFunc, true))
            {
                convert = true;
                valid = true;
            }
        }
        else if(leftType->IsFunctorInterface() && rightType->IsFunctorClass())
        {
            auto leftInterface = leftType->GetDefinition()->ToInterfaceDefinition();
            auto rightClass = rightType->GetDefinition()->ToClassDefinition();

            bool alreadyHasInterface = false;

            for(auto& itf : rightClass->interfaces)
            {
                assert(itf->type);

                if(itf->type == leftType)
                {
                    alreadyHasInterface = true;
                    break;
                }
            }

            if(alreadyHasInterface)
            {
                valid = true;
            }
            else
            {
                auto leftInvoke = leftInterface->GetFunction("invoke");
                auto rightInvoke = rightClass->GetFunction("invoke");

                if(IsFunctionAssignable(leftInvoke, rightInvoke, true))
                {
                    convert = true;
                    valid = true;
                }
            }
        }
        else if(leftType->IsNullable() && rightType->IsNull())
        {
            convert = true;
            valid = true;
        }
        else if(leftType->IsObject() && rightType != nullptr)
        {
            convert = true;
            valid = true;
        }
        else if(leftType->IsNumber() && rightType->IsInteger())
        {
            convert = true;
            valid = true;
        }
        else if(leftType->IsInterface() && rightType->IsClass())
        {
            convert = true;
            valid = true;
        }
        else if(leftType->IsArray() && rightType->IsArray() && leftType->GetElementType()->IsVoid())
        {
            convert = true;
            valid = true;
        }
    }
    else if(operation == TokenType::AddAssign)
    {
        if(leftType == rightType && (leftType->IsInteger() || leftType->IsNumber() || leftType->IsString()))
        {
            valid = true;
        }
        else if(leftType->IsNumber() && rightType->IsInteger())
        {
            convert = true;
            valid = true;
        }
        else if(leftType->IsString() && (rightType->IsBoolean() || rightType->IsInteger() || rightType->IsNumber()))
        {
            convert = true;
            valid = true;
        }
    }
    else if(operation == TokenType::SubAssign || operation == TokenType::MulAssign || operation == TokenType::DivAssign || operation == TokenType::ModAssign)
    {
        if(leftType == rightType && (leftType->IsInteger() || leftType->IsNumber()))
        {
            valid = true;
        }
        else if(leftType->IsNumber() && rightType->IsInteger())
        {
            convert = true;
            valid = true;
        }
    }
    else if(operation == TokenType::LeftShiftAssign ||
        operation == TokenType::RightShiftAssign ||
        operation == TokenType::BitOrAssign ||
        operation == TokenType::BitAndAssign ||
        operation == TokenType::BitXorAssign)
    {
        if(leftType->IsInteger() && rightType->IsInteger())
        {
            valid = true;
        }
    }

    if(needsConversion)
        *needsConversion = convert;

    return valid;
}

void SemanticAnalyzer::ParseCodeString(Scope* enclosingScope, const std::string& code)
{
    auto loc = enclosingScope->owner->loc;

    Lexer lexer(loc.file.view(), code, loc.line, true);
    std::vector<Token> tokens = lexer.Tokenize();
    
    Parser parser(tokens, true);

    ScopeStack scopes;
    scopes.PushFromRoot(enclosingScope);

    parser.Parse(astRoot, scopes);
}

void SemanticAnalyzer::ProcessAssignment(const SourceLocation& leftLoc, Type* leftType, sptr<Expression>& right, TokenType operation)
{
    // ensure types can be evaluated
    Type* rightType = right->EvaluateType();
    ENFORCE(leftType != nullptr, leftLoc, "expression cannot be evaluated");
    ENFORCE(rightType != nullptr, right->loc, "expression cannot be evaluated");

    bool needsConversion;
    bool canAssign = IsAssignable(leftType, rightType, operation, &needsConversion);

    ENFORCE(canAssign, right->loc, "invalid assignment of type {} to {}", rightType->GetName(), leftType->GetName());

    if(needsConversion)
    {
        if(leftType->IsFunctorInterface() && (rightType->IsFunction() || rightType->IsFunctorClass()))
        {
            if(rightType->IsFunction())
            {
                auto funcName = utility::ReplaceAll(rightType->GetName(), ".", "$");
                std::string functorName = std::format("$functorClass${}", funcName);

                auto ident = right->ToIdentifierExpression();
                assert(ident);

                auto func = ident->targetDef->ToFunctionDefinition();
                assert(func);

                sptr<Expression> callTarget = ident->context;

                sptr<ClassDefinition> functorClass;

                // try to look up function class by name
                if(auto def = astRoot->global->scope->FindDefinition(functorName))
                {
                    functorClass = def->ToClassDefinition();
                    assert(functorClass);
                }

                if(!functorClass)
                {
                    std::string returnType { func->returnType->GetTypeName(true) };
                    std::string paramList;
                    std::string argList;

                    auto params = func->GetChildren<ParameterDefinition>();
                    auto paramIt = params.begin();

                    if(func->HasImplicitThisParam())
                        ++paramIt;

                    for( ; paramIt != params.end(); ++paramIt)
                    {
                        const auto& param = *paramIt;

                        auto paramType = param->typeSpec->GetTypeName(true);

                        if(!paramList.empty())
                            paramList += ", ";

                        if(!argList.empty())
                            argList += ", ";

                        paramList += paramType + " " + param->name;
                        argList += param->name;
                    }

                    std::string callTargetTypeName;
                
                    if(callTarget)
                    {
                        auto callTargetType = callTarget->EvaluateType();
                        assert(callTargetType);

                        callTargetTypeName = callTargetType->GetDefinition()->qualifiedName;
                    }

                    constexpr const char* mixinFormat = R""""(
    class {}
    {{
        {}{} target = null;

        {} invoke({})
        {{
            // {{return }}{{target.}}{{func}}({{args}});
            {}{}{}({});
        }}
    }}
    )"""";
                
                    std::string mixinCode = std::format(mixinFormat,
                        functorName,
                        callTarget ? "" : "//",
                        callTargetTypeName,
                        returnType,
                        paramList,
                        !func->returnType->IsVoid() ? "return " : "",
                        callTarget ? "target." : "",
                        callTarget ? func->name : func->qualifiedName,
                        argList
                    );
                
                    ParseCodeString(astRoot->global->scope.get(), mixinCode);

                    auto def = astRoot->global->scope->FindDefinition(functorName);
                    assert(def);

                    functorClass = def->ToClassDefinition();
                    assert(functorClass);

                    functorClass->isFunctor = true;

                    VisitChild(functorClass);
                }

                bool alreadyHasInterface = false;

                for(auto& itf : functorClass->interfaces)
                {
                    assert(itf->type);

                    if(itf->type == leftType)
                    {
                        alreadyHasInterface = true;
                        break;
                    }
                }
            
                if(!alreadyHasInterface)
                {
                    auto functorInterface = leftType->GetDefinition()->ToFunctorInterfaceDefinition();
                    assert(functorInterface);

                    auto itfTypeSpec = spnew<TypeSpecifier>(
                        functorInterface->loc,
                        astRoot->global->scope.get(),
                        functorInterface->qualifiedName );

                    functorClass->interfaces.push_back(itfTypeSpec);
                    functorClass->implementations.clear();
                    VisitChild(functorClass);
                }

                auto newExpr = spnew<NewExpression>(right->loc, right->scope);
                newExpr->typeSpec = spnew<TypeSpecifier>(right->loc, right->scope, functorClass->qualifiedName);
            
                if(callTarget)
                    newExpr->arguments.push_back(callTarget);
            
                right = newExpr;
                VisitChild(right);
                rightType = right->EvaluateType();
            }
            else
            {
                // functor expression
                auto functorInterface = leftType->GetDefinition()->ToFunctorInterfaceDefinition();
                auto functorClass = rightType->GetDefinition()->ToFunctorClassDefinition();

                bool alreadyHasInterface = false;

                for(auto& itf : functorClass->interfaces)
                {
                    if(itf->type == leftType)
                    {
                        alreadyHasInterface = true;
                        break;
                    }
                }

                if(!alreadyHasInterface)
                {
                    auto itfTypeSpec = spnew<TypeSpecifier>(
                        functorInterface->loc,
                        astRoot->global->scope.get(),
                        functorInterface->qualifiedName );

                    functorClass->interfaces.push_back(itfTypeSpec);
                    functorClass->implementations.clear();
                    VisitChild(functorClass);
                }
            }

            // the functor class implements the functor interface, so give the
            // expression the interface type that the assignment target expects
            auto castTypeSpec = spnew<TypeSpecifier>(right->loc, leftType);
            auto castExpr = spnew<CastExpression>(right->loc, right->scope, castTypeSpec, right);
            right = castExpr;
            VisitChild(right);
            rightType = right->EvaluateType();
        }
        else
        {
            if (leftType->IsNullable() && rightType->IsNullable())
            {
                auto resultTypeSpec = spnew<TypeSpecifier>(right->loc, leftType);
                auto castExpr = spnew<CastExpression>(right->loc, right->scope, resultTypeSpec, right);
                right = castExpr;
                VisitChild(right);
                rightType = right->EvaluateType();
            }
            else
            {
                auto resultTypeSpec = spnew<TypeSpecifier>(right->loc, leftType);
                auto convertExpr = spnew<ConvertExpression>(right->loc, right->scope, resultTypeSpec, right);
                right = convertExpr;
                VisitChild(right);
                rightType = right->EvaluateType();
            }
        }
    }
}

void SemanticAnalyzer::ProcessBinaryOperation(const sptr<BinaryExpression>& expr)
{
    SourceLocation loc = expr->loc;
    
    Type* leftType = expr->left->EvaluateType();
    Type* rightType = expr->right->EvaluateType();
    
    ENFORCE(leftType != nullptr, expr->left->loc, "expression cannot be evaluated");
    ENFORCE(rightType != nullptr, expr->right->loc, "expression cannot be evaluated");

    bool valid = false;

    if(expr->operation == TokenType::Add)
    {
        if(leftType == rightType && (leftType->IsNumber() || leftType->IsInteger() || leftType->IsString()))
        {
            valid = true;
        }
        // promote left to number
        else if(leftType->IsInteger() && rightType->IsNumber())
        {
            auto resultTypeSpec = spnew<TypeSpecifier>(expr->left->loc, Type::Get("num"));
            expr->left = spnew<ConvertExpression>(expr->left->loc, expr->left->scope, resultTypeSpec, expr->left);
            VisitChild(expr->left);
            valid = true;
        }
        // promote right to number
        else if(leftType->IsNumber() && rightType->IsInteger())
        {
            auto resultTypeSpec = spnew<TypeSpecifier>(expr->right->loc, Type::Get("num"));
            expr->right = spnew<ConvertExpression>(expr->right->loc, expr->right->scope, resultTypeSpec, expr->right);
            VisitChild(expr->right);
            valid = true;
        }
        // promote left to string
        else if(rightType->IsString() && (leftType->IsBoolean() || leftType->IsInteger() || leftType->IsNumber() || leftType->IsEnum()))
        {
            auto resultTypeSpec = spnew<TypeSpecifier>(expr->left->loc, Type::Get("string"));
            expr->left = spnew<ConvertExpression>(expr->left->loc, expr->left->scope, resultTypeSpec, expr->left);
            VisitChild(expr->left);
            valid = true;
        }
        // promote right to string
        else if(leftType->IsString() && (rightType->IsBoolean() || rightType->IsInteger() || rightType->IsNumber() || rightType->IsEnum()))        {
            auto resultTypeSpec = spnew<TypeSpecifier>(expr->right->loc, Type::Get("string"));
            expr->right = spnew<ConvertExpression>(expr->right->loc, expr->right->scope, resultTypeSpec, expr->right);
            VisitChild(expr->right);
            valid = true;
        }
    }
    else if(expr->operation == TokenType::Sub ||
        expr->operation == TokenType::Mul ||
        expr->operation == TokenType::Div ||
        expr->operation == TokenType::Mod ||
        expr->operation == TokenType::Less ||
        expr->operation == TokenType::LessEqual ||
        expr->operation == TokenType::Greater ||
        expr->operation == TokenType::GreaterEqual)
    {
        if(leftType == rightType && (leftType->IsNumber() || leftType->IsInteger()))
        {
            valid = true;
        }
        // promote left to number
        else if(leftType->IsInteger() && rightType->IsNumber())
        {
            auto resultTypeSpec = spnew<TypeSpecifier>(expr->left->loc, Type::Get("num"));
            expr->left = spnew<ConvertExpression>(expr->left->loc, expr->left->scope, resultTypeSpec, expr->left);
            VisitChild(expr->left);
            valid = true;
        }
        // promote right to number
        else if(leftType->IsNumber() && rightType->IsInteger())
        {
            auto resultTypeSpec = spnew<TypeSpecifier>(expr->right->loc, Type::Get("num"));
            expr->right = spnew<ConvertExpression>(expr->right->loc, expr->right->scope, resultTypeSpec, expr->right);
            VisitChild(expr->right);
            valid = true;
        }
    }
    else if(expr->operation == TokenType::LeftShift ||
        expr->operation == TokenType::RightShift ||
        expr->operation == TokenType::BitOr ||
        expr->operation == TokenType::BitAnd ||
        expr->operation == TokenType::BitTest ||
        expr->operation == TokenType::BitXor ||
        expr->operation == TokenType::BitNot)
    {
        if(leftType->IsInteger() && rightType->IsInteger())
        {
            valid = true;
        }
        else if(leftType->IsEnum() && rightType->IsEnum())
        {
            valid = true;
        }
    }
    else if(expr->operation == TokenType::LogicalAnd ||
        expr->operation == TokenType::LogicalOr)
    {
        if(leftType->IsBoolean() && rightType->IsBoolean())
        {
            valid = true;
        }
    }
    else if(expr->operation == TokenType::Equal ||
        expr->operation == TokenType::NotEqual)
    {
        if(leftType == rightType)
        {
            valid = true;
        }
        // promote left to number
        else if(leftType->IsInteger() && rightType->IsNumber())
        {
            auto resultTypeSpec = spnew<TypeSpecifier>(expr->left->loc, Type::Get("num"));
            expr->left = spnew<ConvertExpression>(expr->left->loc, expr->left->scope, resultTypeSpec, expr->left);
            VisitChild(expr->left);
            valid = true;
        }
        // promote right to number
        else if(leftType->IsNumber() && rightType->IsInteger())
        {
            auto resultTypeSpec = spnew<TypeSpecifier>(expr->right->loc, Type::Get("num"));
            expr->right = spnew<ConvertExpression>(expr->right->loc, expr->right->scope, resultTypeSpec, expr->right);
            VisitChild(expr->right);
            valid = true;
        }
        // demote right to object
        else if(leftType->IsObject() && (rightType->IsArray() || rightType->IsClass() || rightType->IsInterface() || rightType->IsString()))
        {
            auto resultTypeSpec = spnew<TypeSpecifier>(expr->right->loc, Type::Get("object"));
            expr->right = spnew<CastExpression>(expr->right->loc, expr->right->scope, resultTypeSpec, expr->right);
            VisitChild(expr->right);
            valid = true;
        }
        // demote left to object
        else if(rightType->IsObject() && (leftType->IsArray() || leftType->IsClass() || leftType->IsInterface() || leftType->IsString()))
        {
            auto resultTypeSpec = spnew<TypeSpecifier>(expr->left->loc, Type::Get("object"));
            expr->left = spnew<CastExpression>(expr->left->loc, expr->left->scope, resultTypeSpec, expr->left);
            VisitChild(expr->left);
            valid = true;
        }
        // convert right to left's type
        else if(leftType->IsNullable() && rightType->IsNull())
        {
            auto resultTypeSpec = spnew<TypeSpecifier>(expr->right->loc, leftType);
            expr->right = spnew<CastExpression>(expr->right->loc, expr->right->scope, resultTypeSpec, expr->right);
            VisitChild(expr->right);
            valid = true;
        }
        // convert left to right's type
        else if(rightType->IsNullable() && leftType->IsNull())
        {
            auto resultTypeSpec = spnew<TypeSpecifier>(expr->left->loc, rightType);
            expr->left = spnew<CastExpression>(expr->left->loc, expr->left->scope, resultTypeSpec, expr->left);
            VisitChild(expr->left);
            valid = true;
        }
    }

    ENFORCE(valid, loc, "invalid binary operation between types {} and {}", leftType->GetName(), rightType->GetName());
}

void SemanticAnalyzer::ProcessCondition(sptr<Expression>& expr)
{
    ProcessAssignment(expr->loc, Type::Get("bool"), expr);
}

/*****************************
*            ROOT            *
*****************************/

void SemanticAnalyzer::Visit(const sptr<ASTRoot>& node)
{
    astRoot = node;
    auto finally = scope_exit([&]{
        astRoot = nullptr;
    });
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<BasicTypeDefinition>& node) {
    ASTVisitor::Visit(node);
}

sptr<FunctionDefinition> SemanticAnalyzer::GetMatchingFunction(const sptr<ClassDefinition>& classDef, const sptr<InterfaceDefinition>& interfaceDef, const sptr<FunctionDefinition>& interfaceFunc)
{
    sptr<FunctionDefinition> match;
     
    auto itfFuncParams = interfaceFunc->GetChildren<ParameterDefinition>();

    for(auto& def : classDef->scope->FindDefinitions(interfaceFunc->name))
    {
        if(auto func = def->ToFunctionDefinition())
        {
            // a template declaration has no resolved signature to match against
            if(func->IsTemplateDeclaration())
                continue;

            auto funcParams = func->GetChildren<ParameterDefinition>();

            if(funcParams.count() == itfFuncParams.count())
            {
                bool paramsMatch = true;
                size_t i = 0;

                for(auto fp = funcParams.begin(), ifp = itfFuncParams.begin();
                    fp != funcParams.end(); ++fp, ++ifp, ++i)
                {
                    auto& funcParamType = (*fp)->typeSpec;
                    auto& itfFuncParamType = (*ifp)->typeSpec;

                    assert(funcParamType->type);
                    assert(itfFuncParamType->type);

                    if (i == 0)
                    {
                        // TODO: remove this?
                        // The first param should be a class that implements the interface.
                        // We probably don't need to check the first param since implicit "this" is added
                        // automatically and would always be the class that implements the interface.
                        bool firstParamImplementsInterface = false;

                        if (auto contextDef = funcParamType->type->GetDefinition())
                        {
                            if (auto contextClassDef = contextDef->ToClassDefinition())
                            {
                                for (auto& itf : contextClassDef->interfaces)
                                {
                                    if (itf->type == interfaceDef->type)
                                    {
                                        firstParamImplementsInterface = true;
                                        break;
                                    }
                                }
                            }
                        }

                        if (!firstParamImplementsInterface)
                        {
                            paramsMatch = false;
                            break;
                        }
                    }
                    else
                    {
                        if (funcParamType->type != itfFuncParamType->type)
                        {
                            paramsMatch = false;
                            break;
                        }
                    }
                }

                if(paramsMatch)
                {
                    match = func;
                    break;
                }
            }
        }
    }

    return match;
}

void SemanticAnalyzer::Visit(const sptr<ClassDefinition>& node)
{
    if(node->IsTemplateDeclaration())
        return;

    ASTVisitor::Visit(node);

    if(!node->interfaces.empty() && node->implementations.empty())
    {
        for(size_t i = 0; i != node->interfaces.size(); ++i)
        {
            auto& itfTypeSpec = node->interfaces[i];

            auto interfaceDef = itfTypeSpec->GetType()->GetDefinition()->ToInterfaceDefinition();
            VisitChild(interfaceDef);

            // find implementations for this interface
            node->implementations.emplace_back();

            for(auto interfaceFunc : interfaceDef->GetChildren<FunctionDefinition>())
            {
                auto classFunc = GetMatchingFunction(node, interfaceDef, interfaceFunc);
                ENFORCE(!!classFunc, itfTypeSpec->loc, "missing implementation for function: {}", interfaceFunc->name);                
                node->implementations.back().push_back(classFunc.get());
            }
        }
    }
}

void SemanticAnalyzer::Visit(const sptr<EnumDefinition>& node) {
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<EnumMemberDefinition>& node) {
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<FunctionDefinition>& node)
{
    if(node->IsTemplateDeclaration())
        return;

    ASTVisitor::Visit(node);

    if(node->isExternal)
    {
        if(!node->externalFunction && !node->externalIntrinsic)
        {
            std::string name = std::string(node->qualifiedName);

            auto lastDot = name.find_last_of('.');
            if(lastDot != std::string_view::npos)
            {
                if(lastDot < name.length() - 1)
                {
                    char firstNameChar = name[lastDot + 1];
                    if(Lexer::IsInternalSymbol(firstNameChar))
                    {
                        name.erase(name.begin() + lastDot + 1);
                    }
                }
            }
            else
            {
                if(!name.empty() && Lexer::IsInternalSymbol(name[0]))
                    name.erase(name.begin());
            }

            std::string signature { node->returnType->GetTypeName(true) };
            signature += "(";
            
            size_t paramIndex = 0;
            for(const auto& param : node->GetChildren<ParameterDefinition>())
            {
                if(paramIndex++ != 0) signature += ",";
                signature += param->typeSpec->GetTypeName(true);
            }

            signature += ")";

            if(auto func = Compiler::GetActiveCompiler()->GetFunction(name, signature))
            {
                auto paramTypes = func->GetParamTypes();
                auto params = node->GetChildren<ParameterDefinition>();
                ENFORCE(params.count() == paramTypes.size(), node->loc, "Native function parameter count does not match external function declaration: {}", name);
            
                auto currentParam = params.begin();

                for(auto paramType : paramTypes)
                {
                    auto type = (*currentParam)->typeSpec->type;
                    bool isMatch = false;

                    switch(paramType)
                    {
                    case WordType::Boolean:
                        isMatch = type->IsBoolean();
                        break;
                    case WordType::Integer:
                        isMatch = type->IsInteger() || type->IsEnum();
                        break;
                    case WordType::Number:
                        isMatch = type->IsNumber();
                        break;
                    case WordType::String:
                        isMatch = type->IsString();
                        break;
                    case WordType::Array:
                        isMatch = type->IsArray();
                        break;
                    case WordType::Class:
                        isMatch = type->IsClass() || type->IsInterface();
                        break;
                    case WordType::Object:
                        isMatch = type->IsObject() || type->IsClass() || type->IsInterface() || type->IsArray() || type->IsString();
                        break;
                    case WordType::Reference:
                        isMatch = type->IsStruct();
                        break;
                    }

                    ENFORCE(isMatch, node->loc, "External function parameter mismatch: {}", name);
                    ++currentParam;
                }

                {
                    auto returnType = func->GetReturnType();
                    auto type = node->returnType->type;
                    bool isMatch = false;

                    switch(returnType)
                    {
                    case WordType::Boolean:
                        isMatch = type->IsBoolean();
                        break;
                    case WordType::Integer:
                        isMatch = type->IsInteger() || type->IsEnum();
                        break;
                    case WordType::Number:
                        isMatch = type->IsNumber();
                        break;
                    case WordType::String:
                        isMatch = type->IsString();
                        break;
                    case WordType::Array:
                        isMatch = type->IsArray();
                        break;
                    case WordType::Class:
                        isMatch = type->IsClass() || type->IsInterface();
                        break;
                    case WordType::Object:
                        isMatch = type->IsObject() || type->IsClass() || type->IsInterface() || type->IsString() || type->IsArray();
                        break;
                    case WordType::Reference:
                        isMatch = type->IsStruct();
                        break;
                    case WordType::Void:
                        isMatch = type->IsVoid();
                        break;
                    }

                    ENFORCE(isMatch, node->loc, "External function return type mismatch: {}", name);
                }

                node->externalFunction = func;
            }
            else if(auto intrin = Compiler::GetActiveCompiler()->GetIntrinsic(name, signature))
            {
                node->externalIntrinsic = intrin;
            }
            else
            {
                ENFORCE(false, node->loc, "No native function or intrinsic found for extern definition: {}", name);
            }
        }
    }
    else if(node->isAbstract)
    {

    }
    else if(auto owningClass = node->parent->ToClassDefinition();
        owningClass != nullptr && owningClass->isCoroutineState)
    {
        auto valueField = owningClass->GetVariable("$value");
        if(valueField) // $value wouldn't exist for Task<void>
        {
            auto& stmts = node->body->statements;
            
            ENFORCE(!stmts.empty() && stmts.back()->ToReturnStatement(),
                node->loc, "function missing return statement");
        }
    }
    else if(!node->returnType->IsVoid())
    {
        auto& stmts = node->body->statements;

        ENFORCE(!stmts.empty() && (stmts.back()->ToReturnStatement()),
            node->loc, "function missing return statement");
    }
}

void SemanticAnalyzer::Visit(const sptr<InterfaceDefinition>& node)
{
    if(node->IsTemplateDeclaration())
        return;

    ASTVisitor::Visit(node);
}

size_t SemanticAnalyzer::GetVarSize(Type* type)
{
    size_t size = 0;

    if(type->IsStruct())
    {
        auto structDef = type->GetDefinition()->ToStructDefinition();
        assert(structDef);

        for(const auto& field : structDef->GetChildren<VariableDefinition>())
        {
            VisitChild(field->typeSpec);
            size += GetVarSize(field->typeSpec->type);
        }
    }
    else
    {
        size = 1;
    }

    return size;
}

void SemanticAnalyzer::Visit(const sptr<ParameterDefinition>& node)
{
    ASTVisitor::Visit(node);

    auto funcDef = node->parent->ToFunctionDefinition();
    assert(funcDef);
}

void SemanticAnalyzer::Visit(const sptr<PropertyDefinition>& node)
{
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<SectionDefinition>& node) {
    ASTVisitor::Visit(node);

    if(node->statements.empty() || !node->statements.back()->ToReturnStatement())
    {
        auto loc = !node->statements.empty() ? node->statements.back()->loc : node->loc;
        auto ret = spnew<ReturnStatement>(loc, node->scope.get());
        node->statements.push_back(ret);
        VisitChild(ret);
    }
}

void SemanticAnalyzer::Visit(const sptr<StructDefinition>& node)
{
    if(node->IsTemplateDeclaration())
        return;

    ASTVisitor::Visit(node);

    node->size = GetVarSize(node->type);
}

void SemanticAnalyzer::Visit(const sptr<TemplateDefinition>& node) {
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<TemplateParameterDefinition>& node) {
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<VariableDefinition>& node)
{
    VisitChild(node->initializer);

    if(node->typeSpec->baseTypeName == "var")
    {
        ENFORCE(!!node->initializer, node->loc, "var must have an initializer");
        Type* initType = node->initializer->EvaluateType();
        ENFORCE(!!initType, node->loc, "cannot evaluate initializer type: {}", node->name);
        node->typeSpec = spnew<TypeSpecifier>(node->typeSpec->loc, initType);
    }

    VisitChild(node->typeSpec);

    ENFORCE(!node->typeSpec->IsVoid(), node->typeSpec->loc, "cannot declare a variable of type 'void'");

    if(node->initializer)
    {
        auto& typeSpec = node->typeSpec;
        ProcessAssignment(typeSpec->loc, typeSpec->type, node->initializer, TokenType::Assign);
    }
}

// cast<Result>(Type.AsInstance(obj, typeof(Result)))
std::optional<sptr<ASTNode>> SemanticAnalyzer::CreateAsInstanceCall(const sptr<AsExpression>& node)
{
    auto loc = node->loc;
    auto scope = node->scope;
    auto globalScope = astRoot->global->scope.get();

    auto context = spnew<IdentifierExpression>(loc, globalScope, shared_string("Type"));
    auto targetFunc = spnew<IdentifierExpression>(loc, scope, context, shared_string("AsInstance"));

    auto arg1 = node->value;
    auto arg2 = spnew<TypeOfExpression>(loc, scope, node->typeSpec);

    auto callExpr = spnew<CallExpression>(loc, scope, targetFunc);
    callExpr->arguments.push_back(arg1);
    callExpr->arguments.push_back(arg2);

    auto castExpr = spnew<CastExpression>(loc, scope, spnew<TypeSpecifier>(*node->typeSpec), callExpr);
    VisitChild(castExpr);
    
    return castExpr;
}


// String.FromBool(obj)
// String.FromInt(obj)
// String.FromNum(obj)
// String.FromEnum(typeof(OBJ), cast<int>(obj))
std::optional<sptr<ASTNode>> SemanticAnalyzer::CreateStringFromTypeCall(const sptr<AsExpression>& node)
{
    assert(node->typeSpec->type->IsString());

    std::string_view convertFunc;

    Type* valueType = node->value->EvaluateType();
    if (valueType->IsBoolean())
        convertFunc = "FromBool";
    else if (valueType->IsInteger())
        convertFunc = "FromInt";
    else if (valueType->IsNumber())
        convertFunc = "FromNum";
    else if (valueType->IsEnum())
        convertFunc = "FromEnum";

    assert(!convertFunc.empty());
    
    if(convertFunc == "FromEnum")
    {
        auto enumType = spnew<TypeSpecifier>(node->loc, valueType);
        auto arg1 = spnew<TypeOfExpression>(node->loc, node->scope, enumType);
        auto arg2 = spnew<CastExpression>(node->loc, node->scope, spnew<TypeSpecifier>(node->loc, Type::Get("int")), node->value);

        auto context = spnew<IdentifierExpression>(node->loc, astRoot->global->scope.get(), shared_string("String"));
        auto targetFunc = spnew<IdentifierExpression>(node->loc, node->scope, context, shared_string("FromEnum"));
        auto callExpr = spnew<CallExpression>(node->loc, node->scope, targetFunc);
        callExpr->arguments.push_back(arg1);
        callExpr->arguments.push_back(arg2);
        VisitChild(callExpr);
        return callExpr;
    }
    else
    {
        auto context = spnew<IdentifierExpression>(node->loc, astRoot->global->scope.get(), shared_string("String"));
        auto targetFunc = spnew<IdentifierExpression>(node->loc, node->scope, context, shared_string(convertFunc));
        auto callExpr = spnew<CallExpression>(node->loc, node->scope, targetFunc);
        callExpr->arguments.push_back(node->value);
        VisitChild(callExpr);
        return callExpr;
    }
}

void SemanticAnalyzer::Visit(const sptr<AsExpression>& node)
{
    ASTVisitor::Visit(node);

    Type* leftType = node->value->EvaluateType();
    Type* rightType = node->typeSpec->GetType();

    ENFORCE(leftType != nullptr, node->value->loc, "expression cannot be evaluated");
    ENFORCE(rightType != nullptr, node->typeSpec->loc, "expression cannot be evaluated");

    bool valid = false;

    if(leftType == rightType)
    {
        valid = true;
    }
    else if(leftType->IsObject())
    {
        if (rightType->IsBoolean() ||
            rightType->IsInteger() ||
            rightType->IsNumber() ||
            rightType->IsArray() ||
            rightType->IsString() ||
            rightType->IsClass() ||
            rightType->IsInterface())
        {
            valid = true;

            if (rightType->IsNullable())
            {
                replacement = CreateAsInstanceCall(node);
            }
            else
            {
                // unboxing object to a value type requires a non-null instance of the boxed type
                node->value = WrapWithNullCheck(node->value);
                node->value = WrapWithTypeCheck(node->value, rightType);
            }
        }
    }
    else if(leftType->IsInterface())
    {
        if (rightType->IsObject() ||
            rightType->IsClass() ||
            rightType->IsInterface() ||
            rightType->IsBoolean())
        {
            valid = true;
        }

        if (rightType->IsObject() || rightType->IsClass() || rightType->IsInterface())
        {
            replacement = CreateAsInstanceCall(node);
        }
    }
    else if(leftType->IsClass())
    {
        if (rightType->IsObject() ||
            rightType->IsInterface() ||
            rightType->IsBoolean())
        {
            valid = true;
        }

        if (rightType->IsObject() || rightType->IsInterface())
        {
            replacement = CreateAsInstanceCall(node);
        }
    }
    else if(leftType->IsArray() ||
            leftType->IsString())
    {
        if (rightType->IsObject() ||
            rightType->IsBoolean())
        {
            valid = true;
        }

        if (rightType->IsObject())
        {
            replacement = CreateAsInstanceCall(node);
        }
    }
    else if(leftType->IsBoolean())
    {
        if (rightType->IsObject() ||
            rightType->IsInteger() ||
            rightType->IsNumber() ||
            rightType->IsString())
        {
            valid = true;
        }

        if (rightType->IsString())
        {
            replacement = CreateStringFromTypeCall(node);
        }
    }
    else if(leftType->IsInteger())
    {
        if (rightType->IsObject() ||
            rightType->IsBoolean() ||
            rightType->IsNumber() ||
            rightType->IsString())
        {
            valid = true;
        }

        if (rightType->IsString())
        {
            replacement = CreateStringFromTypeCall(node);
        }
    }
    else if(leftType->IsNumber())
    {
        if (rightType->IsObject() ||
            rightType->IsBoolean() ||
            rightType->IsInteger() ||
            rightType->IsString())
        {
            valid = true;
        }

        if (rightType->IsString())
        {
            replacement = CreateStringFromTypeCall(node);
        }
    }
    else if(leftType->IsString())
    {
        if (rightType->IsBoolean() ||
            rightType->IsInteger() ||
            rightType->IsNumber())
        {
            valid = true;
        }
    }
    else if(leftType->IsEnum())
    {
        if (rightType->IsInteger() ||
            rightType->IsString())
        {
            valid = true;
        }

        if (rightType->IsString())
        {
            replacement = CreateStringFromTypeCall(node);
        }
    }

    ENFORCE(valid, node->value->loc, "cannot convert from {} to {}", leftType->GetName(), rightType->GetName());
}

TokenType GetBinOpForAssignment(TokenType assignmentOperator)
{
    TokenType binaryOp = TokenType::Invalid;

    switch(assignmentOperator)
    {
    case TokenType::AddAssign:
        binaryOp = TokenType::Add;
        break;
    case TokenType::SubAssign:
        binaryOp = TokenType::Sub;
        break;
    case TokenType::MulAssign:
        binaryOp = TokenType::Mul;
        break;
    case TokenType::DivAssign:
        binaryOp = TokenType::Div;
        break;
    case TokenType::ModAssign:
        binaryOp = TokenType::Mod;
        break;
    case TokenType::LeftShiftAssign:
        binaryOp = TokenType::LeftShift;
        break;
    case TokenType::RightShiftAssign:
        binaryOp = TokenType::RightShift;
        break;
    case TokenType::BitAndAssign:
        binaryOp = TokenType::BitAnd;
        break;
    case TokenType::BitOrAssign:
        binaryOp = TokenType::BitOr;
        break;
    case TokenType::BitXorAssign:
        binaryOp = TokenType::BitXor;
        break;
    }

    return binaryOp;
}

void SemanticAnalyzer::Visit(const sptr<AssignExpression>& node)
{
    if(node->fieldToInitialize && node->fieldToInitialize->typeSpec->baseTypeName == "var")
    {
        VisitChild(node->right);
        auto rightType = node->right->EvaluateType();
        node->fieldToInitialize->typeSpec = spnew<TypeSpecifier>(node->fieldToInitialize->loc, rightType);
    }

    // lower "left OP-assign right" to "left = left OP right"
    if(node->operation != TokenType::Assign)
    {
        TokenType op = GetBinOpForAssignment(node->operation);

        ScopeStack scopes;
        scopes.PushFromRoot(node->scope);
        auto leftCopy = node->left->Clone(scopes, nullptr)->ToExpression();
        
        node->right = spnew<BinaryExpression>(node->loc, node->scope, op, leftCopy, node->right);
        node->operation = TokenType::Assign;
    }

    // lower "left[index] = right" to "left.operator[](index, right)"
    if(auto indexExpr = node->left->ToIndexExpression())
    {
        // and the target is a class object
        VisitChild(indexExpr->target);

        auto targetType = indexExpr->target->EvaluateType();
        assert(targetType);

        if(targetType->IsClass() || targetType->IsStruct())
        {
            auto targetDef = targetType->GetDefinition();
            VisitChild(indexExpr->arg);
            VisitChild(node->right);
            auto def = FindDefinition(targetDef->scope.get(), "operator[]", { indexExpr->arg, node->right });
            if(def)
            {
                auto operatorDef = def->ToFunctionDefinition();
                ENFORCE(!!operatorDef, def->loc, "operator overload must be a function");

                auto targetFunc = spnew<IdentifierExpression>(
                    indexExpr->target->loc, targetDef->scope.get(), indexExpr->target, operatorDef->name);

                auto callExpr = spnew<CallExpression>(indexExpr->target->loc, indexExpr->target->scope, targetFunc);
                callExpr->arguments.push_back(indexExpr->arg);
                callExpr->arguments.push_back(node->right);

                VisitChild(callExpr);
                replacement = callExpr;
                return;
            }
        }
    }
    // lower "left.Prop = right" to "left.$set_Prop(right)"
    else if(auto ident = node->left->ToIdentifierExpression())
    {
        if(ident->context)
        {
            VisitChild(ident->context);

            auto contextType = ident->context->EvaluateType();
            assert(contextType);

            if(contextType->IsClass() || contextType->IsStruct())
            {
                Scope* scope = contextType->GetDefinition()->scope.get();
                ENFORCE(!!scope, node->loc, "invalid target for member expression");

                auto targetDef = scope->FindDefinition(ident->value);
                auto propertyDef = targetDef ? targetDef->ToPropertyDefinition() : nullptr;
                if(propertyDef)
                {
                    // replace with property setter
                    auto setterFunc = propertyDef->GetSetterFunction();
                    ENFORCE(!!setterFunc, node->left->loc, "no 'set' method defined for property '{}'", propertyDef->name);

                    auto targetFunc = spnew<IdentifierExpression>(node->loc, node->scope, ident->context, setterFunc->name);
                    auto callExpr = spnew<CallExpression>(node->loc, node->scope, targetFunc);
                    callExpr->arguments.push_back(node->right);

                    VisitChild(callExpr);
                    replacement = callExpr;
                    return;
                }
            }
        }
        // need handling of implicit this for properties?
    }

    ASTVisitor::Visit(node);
    ProcessAssignment(node->left->loc, node->left->EvaluateType(), node->right, node->operation);
}

void SemanticAnalyzer::Visit(const sptr<AwaitExpression>& node)
{
    ASTVisitor::Visit(node);

    if(node->expression)
    {
        //auto func = node->expression->scope->owner->ToFunctionDefinition();
        //auto& returnType = func->returnType;
        //ENFORCE(returnType->baseTypeName == "bool", returnType->loc, "await can only be used in a coroutine");
        //ProcessAssignment(returnType->loc, returnType->templateArgs.back()->type, node->expression);
    }
}

std::pair<sptr<FunctionDefinition>, bool> SemanticAnalyzer::GetBinaryOperatorOverload(
    Type* structType, TokenType operation, const sptr<Expression>& left, const sptr<Expression>& right)
{
    auto it = OperatorOverloadNames.find(operation);
    assert(it != OperatorOverloadNames.end());

    assert(structType->IsStruct());
    auto structDef = structType->GetDefinition()->ToStructDefinition();

    bool shouldSwapArgs = false;
    std::vector<sptr<Expression>> args = { left, right };

    auto operatorDef = FindDefinition(structDef->scope.get(), it->second, args);
    if(!operatorDef && operation != TokenType::Equal && operation != TokenType::NotEqual)
    {
        shouldSwapArgs = true;
        args = { right, left };
        operatorDef = FindDefinition(structDef->scope.get(), it->second, args);
    }

    if(!operatorDef)
        return { nullptr, false };

    auto params = operatorDef->GetChildren<ParameterDefinition>();

    ENFORCE(operatorDef->isStatic, operatorDef->loc, "must be static");
    ENFORCE(params.count() == 2, operatorDef->loc, "must have two parameters");
    
    VisitChild(operatorDef);

    auto p = params.begin();
    auto param1 = (*p++);
    auto param2 = (*p++);

    if(operation == TokenType::Equal || operation == TokenType::NotEqual)
    {
        auto paramType1 = param1->typeSpec->type;
        auto paramType2 = param2->typeSpec->type;
        auto paramsValid = paramType1 == structType && paramType2 == structType;
        ENFORCE(paramsValid, operatorDef->loc, "both parameters must be this struct type");
        ENFORCE(operatorDef->returnType->type == Type::Get("bool"), operatorDef->loc, "return type must be bool");
    }
    else
    {
        auto paramType1 = param1->typeSpec->type;
        auto paramType2 = param2->typeSpec->type;
        auto paramsValid = paramType1 == structType || paramType2 == structType;
        ENFORCE(paramsValid, operatorDef->loc, "at least one parameter must be this struct type");
        ENFORCE(operatorDef->returnType->type != Type::Get("void"), operatorDef->loc, "return type cannot be void");
    }

    return { operatorDef, shouldSwapArgs };
}

sptr<FunctionDefinition> SemanticAnalyzer::CreateEqualityOperator(Type* structType, TokenType operation)
{
    assert(structType->IsStruct());

    auto structDef = structType->GetDefinition()->ToStructDefinition();
    auto& typeName = structType->GetName();

    auto isNotOperator = ( operation == TokenType::NotEqual );
    std::string operatorName = isNotOperator ? "operator!=" : "operator==";
    std::string earlyResult = isNotOperator ? "true" : "false";
    std::string finalResult = isNotOperator ? "false" : "true";

    std::string mixinCode;
    mixinCode += std::format("static bool {}({} left, {} right){{\n", operatorName, typeName, typeName);

    for(const auto& field : structDef->GetChildren<VariableDefinition>())
        mixinCode += std::format("  if(left.{} != right.{}) return {};\n", field->name, field->name, earlyResult);

    mixinCode += std::format("  return {};\n}}", finalResult);

    ParseCodeString(structDef->scope.get(), mixinCode);

    auto def = structDef->scope->FindDefinition(operatorName);
    assert(!!def);

    auto operatorDef = def->ToFunctionDefinition();
    assert(!!operatorDef);

    return operatorDef;
}

void SemanticAnalyzer::Visit(const sptr<BinaryExpression>& node)
{
    ASTVisitor::Visit(node);

    Type* leftType = node->left->EvaluateType();
    Type* rightType = node->right->EvaluateType();
    ENFORCE(leftType != nullptr, node->left->loc, "expression cannot be evaluated");
    ENFORCE(rightType != nullptr, node->right->loc, "expression cannot be evaluated");

    if(leftType->IsStruct() || rightType->IsStruct())
    {
        if(node->operation == TokenType::Equal || node->operation == TokenType::NotEqual)
        {
            ENFORCE(leftType == rightType, node->loc, "invalid operand(s) for binary operation");
        }

        if(OverloadableBinaryOperators.contains(node->operation))
        {
            auto structType = leftType->IsStruct() ? leftType : rightType;

            sptr<FunctionDefinition> binaryOperator;
            bool shouldSwapArgs{};

            std::tie(binaryOperator, shouldSwapArgs) = GetBinaryOperatorOverload(structType, node->operation, node->left, node->right);
            
            if(!binaryOperator && rightType->IsStruct())
            {
                structType = rightType;
                std::tie(binaryOperator, shouldSwapArgs) = GetBinaryOperatorOverload(structType, node->operation, node->left, node->right);
            }

            if(!binaryOperator && (node->operation == TokenType::Equal || node->operation == TokenType::NotEqual))
            {
                binaryOperator = CreateEqualityOperator(structType, node->operation);
            }

            if(binaryOperator)
            {
                auto structDef = structType->GetDefinition()->ToStructDefinition();
                auto context = spnew<IdentifierExpression>(node->loc, structDef->parent->scope.get(), structDef->name);
                auto targetFunc = spnew<IdentifierExpression>(node->loc, node->scope, context, binaryOperator->name);
                auto callExpr = spnew<CallExpression>(node->loc, node->scope, targetFunc);
                
                // prevents a stack overflow
                context->targetDef = structDef.get();
                
                if(shouldSwapArgs)
                {
                    callExpr->arguments.push_back(node->right);
                    callExpr->arguments.push_back(node->left);
                }
                else
                {
                    callExpr->arguments.push_back(node->left);
                    callExpr->arguments.push_back(node->right);
                }
                VisitChild(binaryOperator);
                VisitChild(callExpr);
                replacement = callExpr;
                return;
            }
        }
    }

    ProcessBinaryOperation(node);
    
    // now that type checks and promotions have been done...
    leftType = node->left->EvaluateType();
    rightType = node->right->EvaluateType();

    if (leftType->IsString() && rightType->IsString())
    {
        if(node->operation == TokenType::Equal || node->operation == TokenType::NotEqual)
        {
            // lower to String.Equals(left, right) or !String.Equals(left, right)

            auto loc = node->loc;
            auto scope = node->scope;
            auto globalScope = astRoot->global->scope.get();

            auto context = spnew<IdentifierExpression>(loc, globalScope, shared_string("String"));
            auto targetFunc = spnew<IdentifierExpression>(loc, scope, context, shared_string("Equals"));

            auto callExpr = spnew<CallExpression>(loc, scope, targetFunc);
            callExpr->arguments.push_back(node->left);
            callExpr->arguments.push_back(node->right);

            sptr<Expression> expr = callExpr;

            if (node->operation == TokenType::NotEqual)
            {
                expr = spnew<PrefixExpression>(loc, scope, TokenType::LogicalNot, expr);
            }

            VisitChild(expr);
            replacement = expr;
            return;
        }
        else if (node->operation == TokenType::Add)
        {
            // lower to String.Concat(left, right)

            auto loc = node->loc;
            auto scope = node->scope;
            auto globalScope = astRoot->global->scope.get();

            auto context = spnew<IdentifierExpression>(loc, globalScope, shared_string("String"));
            auto targetFunc = spnew<IdentifierExpression>(loc, scope, context, shared_string("Concat"));

            auto callExpr = spnew<CallExpression>(loc, scope, targetFunc);
            callExpr->arguments.push_back(node->left);
            callExpr->arguments.push_back(node->right);

            VisitChild(callExpr);
            replacement = callExpr;
            return;
        }
    }
}

void SemanticAnalyzer::Visit(const sptr<BooleanLiteralExpression>& node) {
    ASTVisitor::Visit(node);
}

sptr<FunctionDefinition> SemanticAnalyzer::FindDefinition(Scope* scope, const std::string& name, const std::vector<sptr<Expression>>& arguments)
{
    std::vector<sptr<FunctionDefinition>> matches;

    for(auto& def : scope->FindDefinitions(name))
    {
        if(auto func = def->ToFunctionDefinition())
        {
            // a template declaration can only be called with explicit template arguments
            if(func->IsTemplateDeclaration())
                continue;

            auto params = func->GetChildren<ParameterDefinition>();
            auto paramCount = params.count();

            auto currentParam = params.begin();

            // skip the implicit 'this' parameter; the context is bound by lookup
            if(func->HasImplicitThisParam())
            {
                ++currentParam;
                --paramCount;
            }

            // reject if arg count doesnt match
            if(paramCount != arguments.size())
                continue;

            auto currentArg = arguments.begin();

            for( ; currentParam != params.end(); ++currentParam, ++currentArg)
            {
                auto param = *currentParam;
                auto arg = *currentArg;

                VisitChild(param);

                auto paramType = param->typeSpec->GetType();
                auto argType = arg->EvaluateType();

                if(!IsAssignable(paramType, argType))
                    break;
            }

            // reject if any arg type doesnt match
            if(currentParam != params.end())
                continue;
            
            // match
            matches.push_back(std::move(func));
        }
    }

    ENFORCE(matches.size() <= 1, scope->owner->loc, "Ambiguous call to '{}'", name);

    return !matches.empty() ? matches.front() : nullptr;
}

sptr<Definition> SemanticAnalyzer::SearchUpward(Scope* scope, const std::string& name, const std::vector<sptr<Expression>>& arguments, Scope* toScope)
{
    sptr<Definition> ret;

    for(auto sc = scope; sc != nullptr; sc = sc->parent)
    {
        auto def = sc->FindDefinition(name);
        if(def)
        {
            if(!def->ToBasicTypeDefinition() || arguments.size() == 1)
            {
                ret = def;
                break;
            }
        }

        if(sc == toScope)
            break;
    }

    return ret;
}

void SemanticAnalyzer::FindCallTargets(std::string_view name, Scope* fromScope, Scope* toScope, std::vector<sptr<Definition>>& callTargets)
{
    for(auto sc = fromScope; sc != nullptr; sc = sc->parent)
    {
        for(auto def : sc->FindDefinitions(name))
        {
            if(auto func = def->ToFunctionDefinition())
            {
                callTargets.push_back(func);
            }
            else if(auto var = def->ToVariableDefinition())
            {
                VisitChild(var->typeSpec);
                
                if(auto varTypeDef = var->typeSpec->type->GetDefinition())
                {
                    if(auto functor = varTypeDef->ToFunctorInterfaceDefinition())
                    {
                        auto targetFunc = functor->GetFunction("invoke");
                        assert(targetFunc);
                        callTargets.push_back(var);
                    }
                }
            }
            else if(auto param = def->ToParameterDefinition())
            {
                VisitChild(param->typeSpec);

                if(auto paramTypeDef = param->typeSpec->type->GetDefinition())
                {
                    if(auto functor = paramTypeDef->ToFunctorInterfaceDefinition())
                    {
                        auto targetFunc = functor->GetFunction("invoke");
                        assert(targetFunc);
                        callTargets.push_back(param);
                    }
                }
            }
            else if(auto prop = def->ToPropertyDefinition())
            {
                VisitChild(prop->typeSpec);

                if(auto propTypeDef = prop->typeSpec->type->GetDefinition())
                {
                    if(auto functor = propTypeDef->ToFunctorInterfaceDefinition())
                    {
                        auto targetFunc = functor->GetFunction("invoke");
                        assert(targetFunc);
                        callTargets.push_back(prop);
                    }
                }
            }
        }

        if(sc == toScope)
            break;
    }
}

std::string GetFunctionParamListString(const sptr<FunctionDefinition>& func)
{
    auto params = func->GetChildren<ParameterDefinition>();
    
    std::string ret;
    ret.reserve(params.count() * 8);

    for(const auto& param : params)
    {
        if(!ret.empty())
            ret += ", ";

        ret += param->typeSpec->GetTypeName();
        ret += " ";
        ret += param->name;
    }

    return ret;
}

sptr<Definition> SemanticAnalyzer::SelectCallTarget(
    std::string_view name,
    const SourceLocation& loc,
    const Scope* scope,
    const std::vector<sptr<Definition>>& callTargets,
    const sptr<Expression>& context,
    const std::vector<sptr<Expression>>& arguments,
    bool suppressErrors)
{
    std::vector<sptr<Definition>> matches;

    for(auto& target : callTargets)
    {
        auto func = GetCallTargetFunction(target);
        assert(func);

        auto params = func->GetChildren<ParameterDefinition>();
        auto paramCount = params.count();

        auto currentParam = params.begin();

        // skip the implicit 'this' parameter; the context is bound by lookup
        if(func->HasImplicitThisParam())
        {
            ++currentParam;
            --paramCount;
        }

        // reject if arg count doesnt match
        if(paramCount != arguments.size())
            continue;

        auto currentArg = arguments.begin();

        for(; currentParam != params.end(); ++currentParam, ++currentArg)
        {
            auto param = *currentParam;
            auto arg = *currentArg;

            VisitChild(param);

            auto paramType = param->typeSpec->GetType();
            auto argType = arg->EvaluateType();

            if(!IsAssignable(paramType, argType))
                break;
        }

        // reject if any arg type didn't match
        if(currentParam != params.end())
            continue;

        // match
        matches.push_back(target);
    }

    sptr<Definition> match;

    if(matches.empty())
    {
        if(!suppressErrors)
        {
            if(callTargets.empty())
            {
                ENFORCE(false, loc, "no definition found for '{}'", name);
            }
            else
            {
                std::string msg = std::format("no matching overload found for '{}':\n", name);
                for(auto& target : callTargets)
                {
                    auto func = GetCallTargetFunction(target);
                    auto paramList = GetFunctionParamListString(func);
                    msg += std::format("-> {}({})\n", target->qualifiedName, paramList);
                }

                ENFORCE(false, loc, "{}", msg);
            }
        }
    }
    else if(matches.size() > 1)
    {
        // favor exact match
        int exactMatch = -1;
        int i = 0;

        for(auto& target : matches)
        {
            auto func = GetCallTargetFunction(target);
            auto params = func->GetChildren<ParameterDefinition>();

            auto currentParam = params.begin();

            // skip the implicit 'this' parameter to stay aligned with the arguments
            if(func->HasImplicitThisParam())
                ++currentParam;

            auto currentArg = arguments.begin();

            for( ; currentParam != params.end(); ++currentParam, ++currentArg)
            {
                auto param = *currentParam;
                auto arg = *currentArg;

                auto paramType = param->typeSpec->GetType();
                auto argType = arg->EvaluateType();

                if(paramType != argType)
                    break;
            }

            if(currentParam == params.end())
            {
                // found an exact match
                if(exactMatch == -1)
                {
                    // found first exact match
                    exactMatch = i;
                }
                else
                {
                    // found multiple exact matches
                    exactMatch = -1;
                    break;
                }
            }

            ++i;
        }

        if(exactMatch != -1)
        {
            // if there's only one exact match, then use it
            match = matches[exactMatch];
        }
        else
        {
            if(!suppressErrors)
            {
                // no exact matches or multiple exact matches, so can't prefer one
                std::string msg = std::format("multiple matching overloads found for '{}':\n", name);
                for(auto& ambiguousMatch : matches)
                {
                    auto func = GetCallTargetFunction(ambiguousMatch);
                    auto paramList = GetFunctionParamListString(func);
                    msg += std::format("-> {}({})\n", ambiguousMatch->qualifiedName, paramList);
                }
                ENFORCE(false, loc, "{}", msg);
            }
        }
    }
    else
    {
        match = matches.front();
    }

    if(!suppressErrors)
    {
        assert(match);

        if(auto func = match->ToFunctionDefinition())
        {
            bool canAccess = func->IsAccessibleFrom(scope);
            ENFORCE(canAccess, loc, "access denied: {}", func->qualifiedName);
        }
        else if(auto var = match->ToVariableDefinition())
        {
            bool canAccess = var->IsAccessibleFrom(scope);
            ENFORCE(canAccess, loc, "access denied: {}", var->qualifiedName);
        }
        else if(auto prop = match->ToPropertyDefinition())
        {
            bool canAccess = prop->IsAccessibleFrom(scope);
            ENFORCE(canAccess, loc, "access denied: {}", prop->qualifiedName);
        }
    }

    return match;
}

sptr<ClassDefinition> SemanticAnalyzer::GetCoroutineOriginalClass(const sptr<ClassDefinition>& coroutineState)
{
    if(!coroutineState->originalClassType)
        return nullptr;

    VisitChild(coroutineState->originalClassType);

    auto type = coroutineState->originalClassType->type;
    return type ? type->GetDefinition()->ToClassDefinition() : sptr<ClassDefinition>{};
}

sptr<FunctionDefinition> SemanticAnalyzer::GetCallTargetFunction(const sptr<Definition>& target)
{
    if(auto func = target->ToFunctionDefinition())
    {
        return func;
    }
    else if(auto functorInterface = target->ToFunctorInterfaceDefinition())
    {
        return functorInterface->GetFunction("invoke");
    }
    else if(auto functorClass = target->ToFunctorClassDefinition())
    {
        return functorClass->GetFunction("invoke");
    }
    else if(auto var = target->ToVariableDefinition())
    {
        if(auto varTypeDef = var->typeSpec->type->GetDefinition())
        {
            if(auto functor = varTypeDef->ToFunctorInterfaceDefinition())
            {
                return functor->GetFunction("invoke");
            }
        }
    }
    else if(auto prop = target->ToPropertyDefinition())
    {
        if(auto propTypeDef = prop->typeSpec->type->GetDefinition())
        {
            if(auto functor = propTypeDef->ToFunctorInterfaceDefinition())
            {
                return functor->GetFunction("invoke");
            }
        }
    }
    else if(auto param = target->ToParameterDefinition())
    {
        if(auto paramTypeDef = param->typeSpec->type->GetDefinition())
        {
            if(auto functor = paramTypeDef->ToFunctorInterfaceDefinition())
            {
                return functor->GetFunction("invoke");
            }
        }
    }

    return nullptr;
}

struct CallTarget
{
    sptr<Definition> target;
    sptr<Expression> context;
};
std::vector<sptr<Definition>> SemanticAnalyzer::FindAllCallTargets(const sptr<IdentifierExpression>& node)
{
    std::vector<sptr<Definition>> callTargets;
    std::vector<CallTarget> callTargets2;
    
    if (node->context)
    {
        Scope* scope{};

        auto contextType = node->context->EvaluateType();
        assert(contextType);

        auto contextDef = contextType->GetDefinition();
        scope = contextDef ? contextDef->scope.get() : nullptr;
        ENFORCE(!!scope, node->loc, "invalid target for member expression");

        FindCallTargets(node->value, scope, scope, callTargets);
    }
    else
    {
        // if identifier is inside a function
        if (auto enclosingFunc = node->scope->owner->ToFunctionDefinition())
        {
            // search up through function locals and params
            FindCallTargets(node->value, node->scope, enclosingFunc->scope.get(), callTargets);

            // search enclosing class
            auto enclosingClass = enclosingFunc->parent->ToClassDefinition();
            if (enclosingClass)
            {
                bool isSpecialFunc = enclosingClass->isCoroutineState && (node->value == "GetValue" || node->value == "Resume");
                if (!isSpecialFunc)
                {
                    auto classScope = enclosingClass->scope.get();
                    FindCallTargets(node->value, classScope, classScope, callTargets);
                }

                // search coroutine enclosure's target class via 'this' reference
                if (enclosingClass->isCoroutineState)
                {
                    if (auto originalClass = GetCoroutineOriginalClass(enclosingClass))
                    {
                        auto classScope = originalClass->scope.get();
                        FindCallTargets(node->value, classScope, classScope, callTargets);
                    }
                }
            }

            // finish searching upward through globals
            auto enclosingDef = enclosingClass ? enclosingClass->parent : enclosingFunc->parent;
            FindCallTargets(node->value, enclosingDef->scope.get(), nullptr, callTargets);
        }
        else
        {
            // just search upward through globals
            FindCallTargets(node->value, node->scope, nullptr, callTargets);
        }
    }

    return callTargets;
}

// Instantiates 'templateFunc' with the template arguments given at the call site, or
// returns the existing instance if one was already created for the same arguments.
sptr<FunctionDefinition> SemanticAnalyzer::InstantiateTemplateFunction(
    const sptr<IdentifierExpression>& node,
    const sptr<FunctionDefinition>& templateFunc)
{
    // the arguments are described with a type specifier so that the instance is named
    // and cloned exactly the same way a class template instance is
    auto instanceType = spnew<TypeSpecifier>(node->loc, node->scope, templateFunc->name, node->templateArgs);

    // A type is registered when a definition is constructed, so an instance that is still
    // being analyzed is already visible here, which is what terminates recursive calls.
    // The type name only identifies the (function name, template arguments) pair, so the
    // originating declaration is what picks this declaration's instance out of the overloads.
    std::string instanceName { templateFunc->qualifiedName };
    instanceName += instanceType->GetTemplateArgs(true);

    for(Type* overload : Type::GetOverloads(instanceName))
    {
        auto instance = overload->GetDefinition()->self()->ToFunctionDefinition();

        if(instance->templateDeclaration == templateFunc.get())
            return instance;
    }

    ScopeStack scopes;
    scopes.PushFromRoot(templateFunc->scope->parent);

    auto instance = templateFunc->Clone(scopes, instanceType)->ToFunctionDefinition();

    VisitChild(instance);

    return instance;
}

// Replaces template function declarations in 'callTargets' with the instances produced
// by the template arguments given at the call site. Template arguments are mandatory,
// so declarations are removed from the candidates when none were provided.
void SemanticAnalyzer::ApplyTemplateArguments(
    const sptr<IdentifierExpression>& node,
    const std::vector<sptr<Expression>>& arguments,
    std::vector<sptr<Definition>>& callTargets)
{
    auto isTemplateDeclaration = [](const sptr<Definition>& target) {
        auto func = target->ToFunctionDefinition();
        return func && func->IsTemplateDeclaration();
    };

    if(node->templateArgs.empty())
    {
        std::erase_if(callTargets, isTemplateDeclaration);
        return;
    }

    for(auto& arg : node->templateArgs)
        VisitChild(arg);

    std::vector<sptr<FunctionDefinition>> candidates;

    for(auto& target : callTargets)
    {
        if(!isTemplateDeclaration(target))
            continue;

        auto templateFunc = target->ToFunctionDefinition();

        auto templateParams = templateFunc->GetChildren<TemplateParameterDefinition>();
        if(templateParams.count() == node->templateArgs.size())
            candidates.push_back(templateFunc);
    }

    ENFORCE(!candidates.empty(), node->loc,
        "no template function named '{}' takes {} template argument(s)", node->value, node->templateArgs.size());

    // Only instantiate viable candidates. If none are viable, instantiate them all for error reporting.
    std::vector<sptr<FunctionDefinition>> viable;

    for(auto& candidate : candidates)
    {
        auto paramCount = candidate->GetChildren<ParameterDefinition>().count();

        // the context is bound once the target is selected, so it isn't an argument yet
        if(candidate->HasImplicitThisParam())
            --paramCount;

        if(paramCount == arguments.size())
            viable.push_back(candidate);
    }

    if(viable.empty())
        viable = candidates;

    callTargets.clear();

    for(auto& candidate : viable)
        callTargets.push_back( InstantiateTemplateFunction(node, candidate) );
}

sptr<Expression> SemanticAnalyzer::WrapWithNullCheck(const sptr<Expression>& value)
{
    auto compiler = Compiler::GetActiveCompiler();
    if(!compiler)
        throw Exception("no active compiler");

    auto valueType = value->EvaluateType();
    if(!compiler->IsNullCheckEnabled() || !valueType->IsNullable())
        return value;

    auto& loc = value->loc;
    auto scope = value->scope;

    auto context = spnew<IdentifierExpression>(loc, astRoot->global->scope.get(), shared_string("Debug"));
    auto targetFunc = spnew<IdentifierExpression>(loc, scope, context, shared_string("AssertNotNull"));
    targetFunc->templateArgs.push_back(spnew<TypeSpecifier>(loc, valueType));

    auto callExpr = spnew<CallExpression>(loc, scope, targetFunc);
    callExpr->arguments.reserve(6);
    callExpr->arguments.push_back(value);
    callExpr->arguments.push_back(spnew<StringLiteralExpression>(loc, scope, shared_string("object reference is null")));
    callExpr->arguments.push_back(spnew<StringLiteralExpression>(loc, scope, loc.file));
    callExpr->arguments.push_back(spnew<IntegerLiteralExpression>(loc, scope, (int64_t)loc.line));
    callExpr->arguments.push_back(spnew<IntegerLiteralExpression>(loc, scope, (int64_t)loc.column));
    callExpr->arguments.push_back(spnew<StringLiteralExpression>(loc, scope, loc.lineText));

    VisitChild(callExpr);
    return callExpr;
}

sptr<Expression> SemanticAnalyzer::WrapWithTypeCheck(const sptr<Expression>& value, Type* primitiveType)
{
    auto compiler = Compiler::GetActiveCompiler();
    if(!compiler)
        throw Exception("no active compiler");

    if(!compiler->IsTypeCheckEnabled())
        return value;

    // unboxing an object stores the primitive in a matching boxed class
    shared_string boxTypeName =
        primitiveType->IsBoolean() ? shared_string("Boolean") :
        primitiveType->IsInteger() ? shared_string("Integer") :
                                     shared_string("Number");

    auto& loc = value->loc;
    auto scope = value->scope;

    auto boxType = spnew<TypeOfExpression>(loc, scope, spnew<TypeSpecifier>(loc, scope, boxTypeName));

    auto context = spnew<IdentifierExpression>(loc, astRoot->global->scope.get(), shared_string("Debug"));
    auto targetFunc = spnew<IdentifierExpression>(loc, scope, context, shared_string("CheckType"));

    auto callExpr = spnew<CallExpression>(loc, scope, targetFunc);
    callExpr->arguments.reserve(7);
    callExpr->arguments.push_back(value);
    callExpr->arguments.push_back(boxType);
    callExpr->arguments.push_back(spnew<StringLiteralExpression>(loc, scope, shared_string(std::format("object is not of type '{}'", boxTypeName))));
    callExpr->arguments.push_back(spnew<StringLiteralExpression>(loc, scope, loc.file));
    callExpr->arguments.push_back(spnew<IntegerLiteralExpression>(loc, scope, (int64_t)loc.line));
    callExpr->arguments.push_back(spnew<IntegerLiteralExpression>(loc, scope, (int64_t)loc.column));
    callExpr->arguments.push_back(spnew<StringLiteralExpression>(loc, scope, loc.lineText));

    VisitChild(callExpr);
    return callExpr;
}

void SemanticAnalyzer::VisitCallTarget(
    const sptr<IdentifierExpression>& node,
    std::vector<sptr<Expression>>& arguments)
{
    if(node->context)
        VisitChild(node->context);

    for(auto& arg : arguments)
        VisitChild(arg);

    if(!node->targetDef)
    {
        std::vector<sptr<Definition>> callTargets = FindAllCallTargets(node);
        ApplyTemplateArguments(node, arguments, callTargets);
        sptr<Definition> targetDef = SelectCallTarget(node->value, node->loc, node->scope, callTargets, node->context, arguments);

        node->targetDef = targetDef.get();
        sptr<FunctionDefinition> func = GetCallTargetFunction(targetDef);
        VisitChild(func->returnType);

        // Unified calling convention: a non-static member receives its instance as the
        // implicit first argument. Now that the target is resolved, bind that context.
        if(func->HasImplicitThisParam())
        {
            sptr<Expression> context;

            // can be a function or a functor expression
            if(auto namedFunction = targetDef->ToFunctionDefinition())
            {
                if(node->context)
                {
                    context = node->context;
                }
                else
                {
                    bool isContextCoroutine = false;

                    if(auto enclosingFunc = node->scope->owner->ToFunctionDefinition())
                    {
                        auto enclosingClass = enclosingFunc->parent->ToClassDefinition();
                        if(enclosingClass && enclosingClass->isCoroutineState)
                        {
                            // for a coroutine state, the context is the original class "$this" reference
                            if(auto originalClass = GetCoroutineOriginalClass(enclosingClass))
                            {
                                if(namedFunction->parent == originalClass.get())
                                    isContextCoroutine = true;
                            }
                        }
                    }

                    auto thisName = isContextCoroutine ? "$this" : "this";
                    context = spnew<IdentifierExpression>(node->loc, node->scope, shared_string(thisName));
                    VisitChild(context);
                }

                // TODO: use this for class references too as a place to put the null check
                // If the context is an r-value struct (like a call returning a struct) then
                // it has no stable address to pass by reference, so store it in a temp first
                auto contextType = context->EvaluateType();
                if (contextType->IsStruct())
                {
                    if (!context->ToIdentifierExpression() &&
                        !context->ToIndexExpression() &&
                        !context->ToFoldExpression() &&
                        !context->ToCachedExpression())
                    {
                        context->pushAsRef = false;

                        auto body = spnew<BlockStatement>(context->loc, node->scope);

                        ScopeStack tempScopes;
                        tempScopes.PushFromRoot(body->scope.get());
                        Scope* foldScope = tempScopes.GetCurrent();

                        // var tmp = context;
                        auto varDefStmt = spnew<VariableDefinitionStatement>(context->loc, foldScope);
                        auto typeSpec = spnew<TypeSpecifier>(context->loc, foldScope, shared_string("var"));
                        varDefStmt->variableDefinition = spnew<VariableDefinition>(context->loc, foldScope, typeSpec, shared_string("tmp"));
                        varDefStmt->variableDefinition->initializer = context;
                        foldScope->AddDefinition(varDefStmt->variableDefinition);
                        body->statements.push_back(varDefStmt);

                        // tmp;
                        auto finalTmpExpr = spnew<IdentifierExpression>(context->loc, foldScope, shared_string("tmp"));
                        finalTmpExpr->pushAsRef = true;  // pushed by reference
                        auto finalTmpStmt = spnew<ExpressionStatement>(finalTmpExpr, foldScope);
                        body->statements.push_back(finalTmpStmt);

                        context = spnew<FoldExpression>(context->loc, node->scope, body);
                        VisitChild(context);
                    }
                    else
                    {
                        context->pushAsRef = true;
                    }
                }
            }
            else // functor variable
            {
                // the resolved function is actually "invoke" of the functor class and the context is the functor itself
                context = spnew<IdentifierExpression>(node->loc, node->scope, node->context, node->value);
                VisitChild(context);
            }

            arguments.insert(arguments.begin(), WrapWithNullCheck(context));
            node->context = nullptr;
        }
    }
}

void SemanticAnalyzer::Visit(const sptr<CachedExpression>& node)
{
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<CastExpression>& node)
{
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<CallExpression>& node)
{
    // when node->target is IdentifierExpression, use ADL for overload resolution instead of default SemanticAnalyzer::Visit
    if(auto ident = node->target->ToIdentifierExpression())
    {
        VisitCallTarget(ident, node->arguments);
    }
    else
    {
        ASTVisitor::Visit(node);

        Type* targetType = node->target->EvaluateType();

        if(targetType->IsFunctorInterface() || targetType->IsFunctorClass())
        {
            auto invokeFunc = GetCallTargetFunction(targetType->GetDefinition()->self());

            if(node->arguments.size() < invokeFunc->GetChildren<ParameterDefinition>().count())
            {
                node->arguments.insert(node->arguments.begin(), WrapWithNullCheck(node->target));
            }
        }
    }

    auto ident = node->target->ToIdentifierExpression();
    if(ident && ident->targetDef)
    {
        // node->arguments now includes the implicit context (if any) as the first
        // argument, so it lines up one-to-one with the target function's parameters.
        auto targetFunc = GetCallTargetFunction(ident->targetDef->self());

        auto params = targetFunc->GetChildren<ParameterDefinition>();
        auto currentParam = params.begin();
        auto currentArg = node->arguments.begin();

        for( ; currentParam != params.end(); ++currentParam, ++currentArg)
        {
            auto& typeSpec = (*currentParam)->typeSpec;
            VisitChild(typeSpec);
            ProcessAssignment(typeSpec->loc, typeSpec->type, (*currentArg));
        }
    }
}

void SemanticAnalyzer::Visit(const sptr<ConvertExpression>& node)
{
    ASTVisitor::Visit(node);

    if (node->resultTypeSpec->type->IsString())
    {
        std::string_view convertFunc;

        Type* valueType = node->value->EvaluateType();
        if (valueType->IsBoolean())
            convertFunc = "FromBool";
        else if (valueType->IsInteger())
            convertFunc = "FromInt";
        else if (valueType->IsNumber())
            convertFunc = "FromNum";
        else if (valueType->IsEnum())
            convertFunc = "FromEnum";

        if (convertFunc == "FromEnum")
        {
            auto enumType = spnew<TypeSpecifier>(node->loc, valueType);
            auto arg1 = spnew<TypeOfExpression>(node->loc, node->scope, enumType);
            auto arg2 = spnew<CastExpression>(node->loc, node->scope, spnew<TypeSpecifier>(node->loc, Type::Get("int")), node->value);

            auto context = spnew<IdentifierExpression>(node->loc, astRoot->global->scope.get(), shared_string("String"));
            auto targetFunc = spnew<IdentifierExpression>(node->loc, node->scope, context, shared_string("FromEnum"));
            auto callExpr = spnew<CallExpression>(node->loc, node->scope, targetFunc);
            callExpr->arguments.push_back(arg1);
            callExpr->arguments.push_back(arg2);
            VisitChild(callExpr);
            replacement = callExpr;
            return;
        }
        else if (!convertFunc.empty())
        {
            auto context = spnew<IdentifierExpression>(node->loc, astRoot->global->scope.get(), shared_string("String"));
            auto targetFunc = spnew<IdentifierExpression>(node->loc, node->scope, context, shared_string(convertFunc));
            auto callExpr = spnew<CallExpression>(node->loc, node->scope, targetFunc);
            callExpr->arguments.push_back(node->value);
            VisitChild(callExpr);
            replacement = callExpr;
            return;
        }
    }

    Type* resultType = node->resultTypeSpec->type;
    if(node->value->EvaluateType()->IsObject() &&
       (resultType->IsBoolean() || resultType->IsInteger() || resultType->IsNumber()))
    {
        // unboxing object to a value type requires a non-null instance of the boxed type
        node->value = WrapWithNullCheck(node->value);
        node->value = WrapWithTypeCheck(node->value, resultType);
    }
}

void SemanticAnalyzer::Visit(const sptr<DefaultValueExpression>& node)
{
    if(node->coroutineFieldInit)
    {
        VisitChild(node->coroutineFieldInit);
        Type* type = node->coroutineFieldInit->right->EvaluateType();
        node->typeSpec = spnew<TypeSpecifier>(node->loc, type);
    }

    VisitChild(node->typeSpec);
}

void SemanticAnalyzer::Visit(const sptr<EmitExpression>& node) {
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<FoldExpression>& node)
{
    assert(node->body);
    ENFORCE(!node->body->statements.empty(), node->body->loc, "body cannot be empty");

    auto finalStatement = node->body->statements.back()->ToExpressionStatement();
    ENFORCE(!!finalStatement, node->body->loc, "final statement must be an expression statement");

    ASTVisitor::Visit(node);
}

sptr<Definition> SemanticAnalyzer::FindIdentifierTarget(const sptr<IdentifierExpression>& node)
{
    sptr<Definition> targetDef;

    if (node->context)
    {
        Scope* scope{};

        auto contextType = node->context->EvaluateType();
        assert(contextType);

        auto contextDef = contextType->GetDefinition();
        scope = contextDef ? contextDef->scope.get() : nullptr;
        ENFORCE(!!scope, node->context->loc, "invalid target for member expression");

        targetDef = scope->FindDefinition(node->value);
    }
    else
    {
        if (auto enclosingFunc = node->scope->owner->ToFunctionDefinition())
        {
            // search function locals and params
            targetDef = Scope::SearchUpward(node->value, node->scope, enclosingFunc->scope);

            // search enclosing class or struct
            auto enclosingClass = enclosingFunc->parent->ToClassDefinition();
            auto enclosingStruct = enclosingFunc->parent->ToStructDefinition();
            auto enclosingScopedDef = utility::FirstTruthy(
                enclosingClass ? enclosingClass->ToDefinition() : nullptr,
                enclosingStruct ? enclosingStruct->ToDefinition() : nullptr);

            if (!targetDef && enclosingScopedDef)
            {
                auto foundDef = enclosingScopedDef->scope->FindDefinition(node->value);
                if (foundDef)
                {
                    if (!enclosingFunc->isStatic ||
                        (foundDef->ToVariableDefinition() && foundDef->ToVariableDefinition()->isStatic) ||
                        (foundDef->ToPropertyDefinition() && foundDef->ToPropertyDefinition()->isStatic))
                    {
                        targetDef = foundDef;
                    }
                }
            }

            // search coroutine enclosure's target class via 'this' reference
            if (!targetDef && enclosingClass && enclosingClass->isCoroutineState)
            {
                if (auto originalClass = GetCoroutineOriginalClass(enclosingClass))
                {
                    targetDef = originalClass->scope->FindDefinition(node->value);
                }
            }

            // finish searching upward through globals
            if (!targetDef)
            {
                auto enclosingDef = enclosingScopedDef ? enclosingScopedDef->parent : enclosingFunc->parent;
                targetDef = enclosingDef->scope->SearchUpward(node->value);
            }
        }
        else
        {
            // just search upward through globals
            targetDef = node->scope->SearchUpward(node->value);
        }
    }

    return targetDef;
}

void SemanticAnalyzer::Visit(const sptr<IdentifierExpression>& node)
{
    if(node->context)
    {
        VisitChild(node->context);

        if(node->context->EvaluateType()->IsArray())
        {
            auto loc = node->loc;
            auto scope = node->scope;

            if(node->value == "Count")
            {
                auto context = spnew<IdentifierExpression>(loc, astRoot->global->scope.get(), shared_string("Array"));
                auto targetFunc = spnew<IdentifierExpression>(loc, scope, context, shared_string("GetCount"));
                auto callExpr = spnew<CallExpression>(loc, scope, targetFunc);
                callExpr->arguments.push_back(node->context);
                VisitChild(callExpr);
                replacement = callExpr;
                return;
            }
            else if(node->value == "Size")
            {
                auto context = spnew<IdentifierExpression>(loc, astRoot->global->scope.get(), shared_string("Array"));
                auto targetFunc = spnew<IdentifierExpression>(loc, scope, context, shared_string("GetSize"));
                auto callExpr = spnew<CallExpression>(loc, scope, targetFunc);
                callExpr->arguments.push_back(node->context);
                VisitChild(callExpr);
                replacement = callExpr;
                return;
            }
        }
    }

    if(!node->targetDef)
    {
        sptr<Definition> targetDef = FindIdentifierTarget(node);
        ENFORCE(!!targetDef, node->loc, "unknown identifier: {}", node->value);

        if(auto func = targetDef->ToFunctionDefinition(); func && func->IsTemplateDeclaration())
        {
            ENFORCE(false, node->loc,
                "call to template function '{}' is missing template arguments", node->value);
        }

        node->targetDef = targetDef.get();

        // add implicit this if needed
        sptr<Expression> context;

        if (node->context)
        {
            context = node->context;
        }
        // this is currently kind of broken because "static" is conflated with meaning !isMember
        else if ((targetDef->ToVariableDefinition() && !targetDef->ToVariableDefinition()->isStatic) ||
                (targetDef->ToPropertyDefinition() && !targetDef->ToPropertyDefinition()->isStatic) ||
                (targetDef->ToFunctionDefinition() && targetDef->ToFunctionDefinition()->HasImplicitThisParam()))
        {
            if (auto enclosingFunc = node->scope->owner->ToFunctionDefinition())
            {
                if (enclosingFunc->HasImplicitThisParam() && targetDef->parent == enclosingFunc->parent)
                {
                    context = spnew<IdentifierExpression>(node->loc, node->scope, shared_string("this"));
                    VisitChild(context);
                }
                else if (auto enclosingClass = enclosingFunc->parent->ToClassDefinition();
                        enclosingClass && enclosingClass->isCoroutineState)
                {
                    // target was reached via the coroutine state's 'this' field
                    if (auto originalClass = GetCoroutineOriginalClass(enclosingClass);
                        originalClass && targetDef->parent == originalClass.get())
                    {
                        context = spnew<IdentifierExpression>(node->loc, node->scope, shared_string("$this"));
                        VisitChild(context);
                    }
                }
            }
        }

        if(context)
        {
            // if the context is an r-value struct (like a call returning a struct) then it has no
            // stable address, so store it in a temp that can be referenced (e.g. via PushRefField)
            auto contextType = context->EvaluateType();
            if(contextType->IsStruct())
            {
                if( !context->ToIdentifierExpression() &&
                    !context->ToIndexExpression() &&
                    !context->ToFoldExpression() &&
                    !context->ToCachedExpression())
                {
                    context->pushAsRef = false;
                    auto cachedVarName = std::format("$temp_2_{}", nextUniqueId++);
                    auto cachedExpression = spnew<CachedExpression>(context, shared_string(std::move(cachedVarName)), true);
                    context = cachedExpression;
                    VisitChild(context);
                }
                else
                {
                    context->pushAsRef = true;
                }
            }
        }

        if(context && Expression::IsValueExpression(context))
        {
            if(auto memberVar = targetDef->ToVariableDefinition(); memberVar && !memberVar->isStatic)
                context = WrapWithNullCheck(context);
        }

        node->context = context;
        if(auto targetParentClassDef = targetDef->parent->ToClassDefinition())
        {
            VisitChild(targetParentClassDef->originalClassType);
        }

        bool canAccess = targetDef->IsAccessibleFrom(node->scope);
        ENFORCE(canAccess, node->loc, "access denied: {}", targetDef->qualifiedName);

        if(auto propertyDef = targetDef->ToPropertyDefinition())
        {
            auto getterFunc = propertyDef->GetGetterFunction();
            ENFORCE(!!getterFunc, node->loc, "no 'get' method defined for property '{}'", propertyDef->name);

            auto targetFunc = spnew<IdentifierExpression>(node->loc, node->scope, node->context, getterFunc->name);
            auto callExpr = spnew<CallExpression>(node->loc, node->scope, targetFunc);
            VisitChild(callExpr);
            replacement = callExpr;
            return;
        }

        node->targetDef = targetDef.get();
        VisitChild(targetDef);
    }
}

void SemanticAnalyzer::Visit(const sptr<IndexExpression>& node)
{
    ASTVisitor::Visit(node);

    // if lhs is an object, then attempt lowering to operator overload function call
    auto targetType = node->target->EvaluateType();
    assert(targetType);

    if(targetType->IsClass() || targetType->IsStruct())
    {
        auto targetDef = targetType->GetDefinition();
        auto def = FindDefinition(targetDef->scope.get(), "operator[]", { node->arg });
        if(def)
        {
            auto operatorDef = def->ToFunctionDefinition();
            ENFORCE(!!operatorDef, def->loc, "operator overload must be a function");

            auto targetFunc = spnew<IdentifierExpression>(node->target->loc, node->target->scope, node->target, operatorDef->name);
            auto callExpr = spnew<CallExpression>(node->target->loc, node->target->scope, targetFunc);
            callExpr->arguments.push_back(node->arg);

            VisitChild(callExpr);
            replacement = callExpr;
        }
    }

    auto IsAlreadyBoundsChecked = [](const sptr<Expression>& expr) {
        if(auto call = expr->ToCallExpression())
            if(auto ident = call->target->ToIdentifierExpression())
                return ident->value == "CheckBounds";
        return false;
    };

    if(targetType->IsArray() && !IsAlreadyBoundsChecked(node->target))
    {
        node->target = WrapWithNullCheck(node->target);

        if(Compiler::GetActiveCompiler()->IsBoundsCheckEnabled())
        {
            auto loc = node->loc;
            auto scope = node->scope;
            auto argLoc = node->arg->loc;

            // evaluate the index once, then share it between the check and the access
            auto cachedIndex = spnew<CachedExpression>(node->arg, shared_string(std::format("$index_{}", nextUniqueId++)));

            // CheckBounds returns the array so the target is also evaluated once
            auto context = spnew<IdentifierExpression>(loc, astRoot->global->scope.get(), shared_string("Array"));
            auto targetFunc = spnew<IdentifierExpression>(loc, scope, context, shared_string("CheckBounds"));
            targetFunc->templateArgs.push_back(spnew<TypeSpecifier>(loc, targetType->GetElementType()));

            auto callExpr = spnew<CallExpression>(loc, scope, targetFunc);
            callExpr->arguments.reserve(7);
            callExpr->arguments.push_back(node->target);
            callExpr->arguments.push_back(cachedIndex);
            callExpr->arguments.push_back(spnew<StringLiteralExpression>(loc, scope, shared_string("index out of range")));
            callExpr->arguments.push_back(spnew<StringLiteralExpression>(loc, scope, argLoc.file));
            callExpr->arguments.push_back(spnew<IntegerLiteralExpression>(loc, scope, (int64_t)argLoc.line));
            callExpr->arguments.push_back(spnew<IntegerLiteralExpression>(loc, scope, (int64_t)argLoc.column));
            callExpr->arguments.push_back(spnew<StringLiteralExpression>(loc, scope, argLoc.lineText));

            node->target = callExpr;
            node->arg = spnew<IdentifierExpression>(loc, scope, cachedIndex->value->value);

            VisitChild(node->target);
            VisitChild(node->arg);
        }
    }
}

void SemanticAnalyzer::Visit(const sptr<IntegerLiteralExpression>& node) {
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<IsExpression>& node)
{
    ASTVisitor::Visit(node);

    Type* valueType = node->value->EvaluateType();
    Type* targetType = node->typeSpec->GetType();

    ENFORCE(valueType != nullptr, node->value->loc, "expression cannot be evaluated");
    ENFORCE(targetType != nullptr, node->typeSpec->loc, "expression cannot be evaluated");

    if (valueType->IsNullable())
    {
        auto loc = node->loc;
        auto scope = node->scope;
        auto globalScope = astRoot->global->scope.get();

        shared_string typeName = node->typeSpec->GetTypeName(true);

        auto context = spnew<IdentifierExpression>(loc, globalScope, shared_string("Type"));
        auto targetFunc = spnew<IdentifierExpression>(loc, scope, context, shared_string("IsInstance"));
        
        auto arg1 = node->value;
        auto arg2 = spnew<TypeOfExpression>(loc, scope, node->typeSpec);

        auto callExpr = spnew<CallExpression>(loc, scope, targetFunc);
        callExpr->arguments.push_back(arg1);
        callExpr->arguments.push_back(arg2);

        VisitChild(callExpr);
        replacement = callExpr;
    }
}

void SemanticAnalyzer::Visit(const sptr<NewExpression>& node)
{
    ASTVisitor::Visit(node);

    if(node->typeSpec->type->IsArray())
    {
        auto loc = node->typeSpec->loc;
        auto elementType = node->typeSpec->type->GetElementType();

        if(node->argumentExpression)
        {
            ENFORCE(node->arguments.empty(), node->loc,
                "array initializer cannot use both a length expression and an initializer list");

            ProcessAssignment(loc, Type::Get("int"), node->argumentExpression);
            Type* argType = node->argumentExpression->EvaluateType();
            ENFORCE(argType == Type::Get("int"), node->loc, "array length expression must be an int");
        }
        else
        {
            for(size_t i = 0; i != node->arguments.size(); ++i)
            {
                ProcessAssignment(loc, elementType, node->arguments[i]);
            }
        }
    }
    else if(auto targetType = node->typeSpec->type; targetType->IsClass() || targetType->IsStruct())
    {
        Definition* def = targetType->GetDefinition();
        Scope* scope = nullptr;

        if(auto classDef = targetType->GetDefinition()->ToClassDefinition())
            scope = classDef->scope.get();
        else if(auto structDef = targetType->GetDefinition()->ToStructDefinition())
            scope = structDef->scope.get();

        assert(scope);

        if(!node->hasConstructor.has_value())
        {
            std::vector<sptr<Definition>> callTargets;
            FindCallTargets("#this", scope, scope, callTargets);
            sptr<Definition> initDef = SelectCallTarget("#this", node->loc, node->scope, callTargets, nullptr, node->arguments, true);
            node->hasConstructor = !!initDef;

            // lower to constructor call: ctor(args) or ctor(newExpr, args)
            if(node->hasConstructor.value())
            {
                // Remove args from the new expression so it's just a stub that
                // can be used to initialize the object.
                std::vector<sptr<Expression>> ctorArgs = std::move(node->arguments);
                node->arguments.clear();

                sptr<Expression> newExpr;

                // If the ctor is not external, we add the 'new' expression as the first
                // ctor arg, but with none of the caller-provided arguments so it will
                // just do the allocation. The caller-provided args are passed to the ctor.
                auto ctor = GetCallTargetFunction(initDef);
                if (!ctor->isExternal)
                {
                    newExpr = node;

                    // wrap in a fold expression if it's a struct so the arg will be passed by ref
                    if (targetType->IsStruct())
                    {
                        // wrap new struct in a fold expression
                        auto body = spnew<BlockStatement>(node->loc, node->scope);

                        ScopeStack scopes;
                        scopes.PushFromRoot(body->scope.get());
                        Scope* foldScope = scopes.GetCurrent();

                        // var tmp = newExpr;
                        auto varDefStmt = spnew<VariableDefinitionStatement>(node->loc, foldScope);
                        auto typeSpec = spnew<TypeSpecifier>(node->loc, foldScope, shared_string("var"));
                        varDefStmt->variableDefinition = spnew<VariableDefinition>(node->loc, foldScope, typeSpec, shared_string("tmp"));
                        varDefStmt->variableDefinition->initializer = newExpr;
                        foldScope->AddDefinition(varDefStmt->variableDefinition);
                        body->statements.push_back(varDefStmt);

                        // tmp;
                        auto finalTmpExpr = spnew<IdentifierExpression>(node->loc, foldScope, shared_string("tmp"));
                        finalTmpExpr->pushAsRef = true;
                        auto finalTmpStmt = spnew<ExpressionStatement>(finalTmpExpr, foldScope);
                        body->statements.push_back(finalTmpStmt);

                        auto fold = spnew<FoldExpression>(node->loc, node->scope, body);
                        VisitChild(fold);
                        newExpr = fold;
                    }
                }

                // replace the NewExpression with a CallExpresssion to the ctor, passing the 
                // args that were initially provided to the NewExpression. The ctor call will
                // return the new object. For non-external constructors, we use the new expression
                // as the identifier context, making this `new T{}.#this(args)`
                auto targetFunc = spnew<IdentifierExpression>(node->loc, scope, newExpr, shared_string("#this"));
                //targetFunc->targetDef = initDef.get();
                auto callExpr = spnew<CallExpression>(node->loc, node->scope, targetFunc, std::move(ctorArgs));
                VisitChild(callExpr);
                replacement = callExpr;
                return;
            }
        }

        // If there's no constructor, process the args as field initializers.
        if(!node->hasConstructor.value())
        {
            auto fields = def->GetChildren<VariableDefinition>();

            ENFORCE(node->arguments.size() <= fields.count(), node->loc, "too many initializers for class");

            auto it = fields.begin();

            for(size_t i = 0; i != node->arguments.size(); ++i)
            {
                auto field = *it++;
                VisitChild(field->typeSpec);
                ProcessAssignment(field->loc, field->typeSpec->type, node->arguments[i]);
            }
        }
    }
    else
    {
        ENFORCE(false, node->loc, "expected class, struct or array element type");
    }
}

void SemanticAnalyzer::Visit(const sptr<NullLiteralExpression>& node) {
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<NumberLiteralExpression>& node) {
    ASTVisitor::Visit(node);
}

sptr<FunctionDefinition> SemanticAnalyzer::GetUnaryOperatorOverload(
    Type* structType, TokenType operation, const sptr<Expression>& value)
{
    auto it = OperatorOverloadNames.find(operation);
    assert(it != OperatorOverloadNames.end());

    assert(structType->IsStruct());
    auto structDef = structType->GetDefinition()->ToStructDefinition();

    auto operatorDef = FindDefinition(structDef->scope.get(), it->second, { value });
    if(!operatorDef)
        return nullptr;

    VisitChild(operatorDef);

    auto params = operatorDef->GetChildren<ParameterDefinition>();
    
    ENFORCE(operatorDef->isStatic, operatorDef->loc, "must be static");
    ENFORCE(!operatorDef->isExternal, operatorDef->loc, "cannot be external");
    ENFORCE(params.count() == 1, operatorDef->loc, "must have one parameter");
    ENFORCE(params.front()->typeSpec->type == structType, operatorDef->loc, "parameter must be this struct type");
    ENFORCE(operatorDef->returnType->type == structType, operatorDef->loc, "return type must be this struct type");
    
    return operatorDef;
}

void SemanticAnalyzer::Visit(const sptr<PostfixExpression>& node)
{
    ASTVisitor::Visit(node);

    Type* argType = node->arg->EvaluateType();
    ENFORCE(!!argType, node->arg->loc, "expression cannot be evaluated");

    if(argType->IsStruct())
    {
        if(OverloadablePostfixOperators.contains(node->operation))
        {
            // lower "arg OP" to "fold{ tmp = arg; OP arg; tmp; }"
            auto unaryOperator = GetUnaryOperatorOverload(argType, node->operation, node->arg);
            ENFORCE(!!unaryOperator, node->loc, "'{}' is not overloaded for '{}'", OperatorOverloadNames.at(node->operation), argType->GetName());

            // create fold expression body
            auto body = spnew<BlockStatement>(node->loc, node->scope);

            ScopeStack scopes;
            scopes.PushFromRoot(body->scope.get());
            Scope* foldScope = scopes.GetCurrent();
            
            // TYPE tmp = arg;
            auto varDefStmt = spnew<VariableDefinitionStatement>(node->loc, foldScope);
            auto typeSpec = spnew<TypeSpecifier>(node->loc, foldScope, argType->GetName());
            varDefStmt->variableDefinition = spnew<VariableDefinition>(node->loc, foldScope, typeSpec, shared_string("tmp"));
            varDefStmt->variableDefinition->initializer = node->arg->Clone(scopes, nullptr)->ToExpression();
            foldScope->AddDefinition(varDefStmt->variableDefinition);

            body->statements.push_back(varDefStmt);

            // OP arg;
            auto prefixOpExpr = spnew<PrefixExpression>(node->loc, foldScope, node->operation, node->arg);
            auto prefixOpStmt = spnew<ExpressionStatement>(prefixOpExpr, foldScope);
            body->statements.push_back(prefixOpStmt);

            // tmp;
            auto finalTmpExpr = spnew<IdentifierExpression>(node->loc, foldScope, shared_string("tmp"));
            auto finalTmpStmt = spnew<ExpressionStatement>(finalTmpExpr, foldScope);
            body->statements.push_back(finalTmpStmt);

            auto fold = spnew<FoldExpression>(node->loc, node->scope, body);

            VisitChild(fold);
            replacement = fold;
        }
    }
}

void SemanticAnalyzer::Visit(const sptr<PrefixExpression>& node)
{
    ASTVisitor::Visit(node);

    Type* argType = node->arg->EvaluateType();
    ENFORCE(!!argType, node->arg->loc, "expression cannot be evaluated");

    if(argType->IsStruct())
    {
        if(OverloadablePrefixOperators.contains(node->operation))
        {
            // lower "OP arg" to "operator OP(arg)" or "(arg = operator OP(arg))"
            auto unaryOperator = GetUnaryOperatorOverload(argType, node->operation, node->arg);
            ENFORCE(!!unaryOperator, node->loc, "'{}' is not overloaded for '{}'", OperatorOverloadNames.at(node->operation), argType->GetName());

            auto structDef = argType->GetDefinition()->ToStructDefinition();
            auto context = spnew<IdentifierExpression>(node->loc, structDef->parent->scope.get(), structDef->name);
            auto targetFunc = spnew<IdentifierExpression>(node->loc, node->scope, context, unaryOperator->name);
            auto callExpr = spnew<CallExpression>(node->loc, node->scope, targetFunc);
            callExpr->arguments.push_back(node->arg);

            // prevents a stack overflow
            context->targetDef = structDef.get();

            if(node->operation == TokenType::Increment || node->operation == TokenType::Decrement)
            {
                ScopeStack scopes;
                scopes.PushFromRoot(node->scope);
                auto argCopy = node->arg->Clone(scopes, nullptr)->ToExpression();
                auto assignExpr = spnew<AssignExpression>(node->loc, node->scope, TokenType::Assign, argCopy, callExpr);

                VisitChild(assignExpr);
                replacement = assignExpr;
            }
            else
            {
                VisitChild(callExpr);
                replacement = callExpr;
            }
        }
    }
}

void SemanticAnalyzer::Visit(const sptr<SizeOfExpression>& node) {
    ASTVisitor::Visit(node);
}


void SemanticAnalyzer::Visit(const sptr<StringLiteralExpression>& node) {
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<TernaryExpression>& node)
{
    ASTVisitor::Visit(node);

    Type* trueValueType = node->trueValue->EvaluateType();
    Type* falseValueType = node->falseValue->EvaluateType();

    ENFORCE(!!trueValueType, node->trueValue->loc, "expression cannot be evaluated");
    ENFORCE(!!falseValueType, node->falseValue->loc, "expression cannot be evaluated");

    ProcessAssignment(node->loc, Type::Get("bool"), node->condition);
}

void SemanticAnalyzer::Visit(const sptr<TypeLiteralExpression>& node) {
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<TypeOfExpression>& node)
{
    ASTVisitor::Visit(node);

    auto loc = node->loc;
    auto scope = node->scope;
    auto globalScope = astRoot->global->scope.get();

    shared_string typeName = node->typeSpec->type->GetName();
    
    auto context = spnew<IdentifierExpression>(loc, globalScope, shared_string("Type"));
    auto targetFunc = spnew<IdentifierExpression>(loc, scope, context, shared_string("Find"));
    auto arg = spnew<TypeLiteralExpression>(loc, globalScope, typeName);

    auto callExpr = spnew<CallExpression>(loc, scope, targetFunc);
    callExpr->arguments.push_back(arg);

    VisitChild(callExpr);
    replacement = callExpr;
}

void SemanticAnalyzer::Visit(const sptr<TypeSpecifier>& node)
{
    ASTVisitor::Visit(node);

    if(!node->type)
    {
        // find definition for base name
        sptr<Definition> definition;
        
        for(auto part : utility::TypeNameSplitter(node->baseTypeName))
        {
            std::string ident = std::string(part);

            if(!definition)
            {
                definition = node->scope->SearchUpward(ident);
                ENFORCE(!!definition, node->loc, "invalid type specifier: {}", ident);
            }
            else
            {
                definition = definition->scope->FindDefinition(ident);
                ENFORCE(!!definition, node->loc, "invalid type specifier: {}", ident);
            }
        }

        if(!node->templateArgs.empty())
        {
            // update name to be fully qualified
            node->baseTypeName = definition->qualifiedName;

            // instantiate template or find existing one
            auto templateDef = definition->ToTemplateDefinition();
            ENFORCE(!!templateDef, node->loc, "definition is not a template: {}", definition->name);
            ENFORCE(!templateDef->ToFunctionDefinition(), node->loc, "a template function cannot be used as a type: {}", definition->name);
            ENFORCE(!templateDef->IsTemplateInstance(), node->loc, "template already instantiated: {}", templateDef->name);

            auto templateParams = templateDef->GetChildren<TemplateParameterDefinition>();
            ENFORCE(templateParams.count() == node->templateArgs.size(), node->loc, "arguments don't match template parameters");

            std::string typeName { node->GetElementTypeName(true) };

            if(Type* instanceType = Type::Get(typeName))
            {
                // use existing instance with matching args
                definition = instanceType->GetDefinition()->self();
            }
            else
            {
                ScopeStack scopes;
                scopes.PushFromRoot(templateDef->scope->parent);

                auto instance = templateDef->Clone(scopes, node)->ToTemplateDefinition();
                VisitChild(instance);

                definition = instance;
            }
        }
        else
        {
            // if base name referred to a template parameter, bubble up through the instantiation
            // to find the actual type and update this type specifier
            auto templateParamDef = definition->ToTemplateParameterDefinition();
            while(templateParamDef)
            {
                ENFORCE(node->templateArgs.empty(), node->loc, "a template parameter cannot be used as a template");
                
                VisitChild(templateParamDef->typeSpec);

                auto nestedType = templateParamDef->typeSpec->type;

                while(nestedType->IsArray())
                {
                    ++node->arrayDimensions;
                    nestedType = nestedType->GetElementType();
                }

                definition = nestedType->GetDefinition()->self();
                templateParamDef = definition->ToTemplateParameterDefinition();
            }

            // update name to be fully qualified
            node->baseTypeName = definition->qualifiedName;
        }

        Type* type = Type::Get(definition.get());

        for(int i = 0; i != node->arrayDimensions; ++i)
            type = type->ArrayOf();

        node->type = type;
    }
}

void SemanticAnalyzer::Visit(const sptr<AssertStatement>& node)
{
    ASTVisitor::Visit(node);
    
    Type* conditionType = node->condition->EvaluateType();
    ENFORCE(conditionType->IsBoolean(), node->condition->loc, "expected boolean condition");

    Type* messageType = node->message->EvaluateType();
    ENFORCE(messageType->IsString() || messageType->IsNull(), node->message->loc, "expected string message or null");

    auto compiler = Compiler::GetActiveCompiler();
    if (!compiler)
        throw Exception("no active compiler");

    if (compiler->IsAssertEnabled())
    {
        auto context = spnew<IdentifierExpression>(node->loc, astRoot->global->scope.get(), shared_string("Debug"));
        auto targetFunc = spnew<IdentifierExpression>(node->loc, node->enclosingScope, context, shared_string("Assert"));

        auto callExpr = spnew<CallExpression>(node->loc, node->enclosingScope, targetFunc);
        callExpr->arguments.reserve(6);
        callExpr->arguments.push_back(node->condition);
        callExpr->arguments.push_back(node->message);
        callExpr->arguments.push_back(spnew<StringLiteralExpression>(node->loc, node->enclosingScope, node->loc.file));
        callExpr->arguments.push_back(spnew<IntegerLiteralExpression>(node->loc, node->enclosingScope, (int64_t)node->loc.line));
        callExpr->arguments.push_back(spnew<IntegerLiteralExpression>(node->loc, node->enclosingScope, (int64_t)node->loc.column));
        callExpr->arguments.push_back(spnew<StringLiteralExpression>(node->loc, node->enclosingScope, node->loc.lineText));

        auto exprStmt = spnew<ExpressionStatement>(callExpr, node->enclosingScope);
        VisitChild(exprStmt);
        replacement = exprStmt;
    }
    else
    {
        auto emptyStmt = spnew<EmptyStatement>(node->loc, node->enclosingScope);
        VisitChild(emptyStmt);
        replacement = emptyStmt;
    }
}

void SemanticAnalyzer::Visit(const sptr<EmptyStatement>& node)
{
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<GotoStatement>& node)
{
    ASTVisitor::Visit(node);
}

void SemanticAnalyzer::Visit(const sptr<ReturnStatement>& node)
{
    ASTVisitor::Visit(node);
    
    if(node->expression)
    {
        if(node->context)
        {
            Type* contextType = node->context->EvaluateType();
            sptr<ClassDefinition> classDef = contextType->GetDefinition()->ToClassDefinition();
            auto valueField = classDef->GetVariable("$value");
            ENFORCE(!!valueField, node->loc, "Return type is void");
            ProcessAssignment(node->loc, valueField->typeSpec->type, node->expression);
        }
        else
        {
            auto func = node->expression->scope->owner->ToFunctionDefinition();
            auto& returnType = func->returnType;
            ProcessAssignment(returnType->loc, returnType->type, node->expression);
        }
    }
}

} // fraze
