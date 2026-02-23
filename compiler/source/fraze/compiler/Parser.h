/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#pragma once
#include <array>
#include <cassert>
#include <format>
#include <memory>
#include <print>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <fraze/compiler/Lexer.h>
#include <fraze/common/Exception.h>
#include <fraze/common/Pointers.h>
#include <fraze/common/Scope.h>
#include <fraze/ast/AST.h>

namespace fraze {

class Parser
{
    const std::vector<Token>& tokens;
    size_t index = 0;
    Token token;
    ScopeStack scopes;
    sptr<ASTRoot> astRoot;
    int nextUniqueId = 0;
public:

    Parser(const std::vector<Token>& tokens)
        : tokens(tokens)
    {
        assert(!tokens.empty());
        token = tokens[0];

        while(IsAnyOf(token.type, TokenType::LineComment, TokenType::BlockComment)
            && !token.IsType(TokenType::EndOfFile))
        {
            ENFORCE(index < tokens.size() - 1, SourceLocation(), "index out of bounds");
            token = tokens[++index];
        }
    }

    const Token& SkipTokenUnchecked()
    {
        const Token& currentToken = tokens[index];

        do {
            token = tokens[++index];
        } while(IsAnyOf(token.type, TokenType::LineComment, TokenType::BlockComment));
        
        return currentToken;
    }

    const Token& Consume()
    {
        TokenType expectedTokenType = token.type;

        ENFORCE(index < tokens.size() - 1, SourceLocation(), "index out of bounds");
        ENFORCE(token.type != TokenType::Invalid, token.loc, "invalid token");
        ENFORCE(token.type != TokenType::EndOfFile, token.loc, "unexpected end of file");

        return SkipTokenUnchecked();
    }

    const Token& Consume(TokenType expectedTokenType)
    {
        ENFORCE(index < tokens.size() - 1, SourceLocation(), "index out of bounds");
        ENFORCE(expectedTokenType != TokenType::Invalid, token.loc, "expected token is invalid");
        ENFORCE(expectedTokenType != TokenType::EndOfFile, token.loc, "expected token is EOF");
        ENFORCE(token.type == expectedTokenType, token.loc, "expected '{}'", Lexer::GetTokenName(expectedTokenType));

        return SkipTokenUnchecked();
    }

    const Token& Consume(Keyword expectedKeyword)
    {
        ENFORCE(index < tokens.size() - 1, SourceLocation(), "index out of bounds");
        ENFORCE(token.IsKeyword(expectedKeyword), token.loc, "expected '{}'", Lexer::GetKeywordName(expectedKeyword));
        return SkipTokenUnchecked();
    }

    bool TryConsume(TokenType expectedTokenType)
    {
        ENFORCE(index < tokens.size() - 1, SourceLocation(), "index out of bounds");
        ENFORCE(expectedTokenType != TokenType::Invalid, token.loc, "expected token is invalid");
        ENFORCE(expectedTokenType != TokenType::EndOfFile, token.loc, "expected token is EOF");

        if(token.type != expectedTokenType)
            return false;

        SkipTokenUnchecked();
        return true;
    }

    bool TryConsume(Keyword expectedKeyword)
    {
        ENFORCE(index < tokens.size() - 1, SourceLocation(), "index out of bounds");
        
        if(!token.IsKeyword(expectedKeyword))
            return false;

        SkipTokenUnchecked();
        return true;
    }

    template<class T, class... Types>
    bool IsAnyOf(T a, Types... bs)
    {
        std::array<T, sizeof...(Types)> bArray = { bs... };

        for(auto b : bArray)
        {
            if(a == b)
                return true;
        }

        return false;
    }

    class Iterator
    {
        std::vector<Token>::const_iterator it;
    public:
        Iterator(std::vector<Token>::const_iterator it)
            : it(it){}

        Iterator& operator++()
        {
            do {
                ++it;
            } while(it->type == TokenType::LineComment || it->type == TokenType::BlockComment);

            return *this;
        }

        Iterator operator++(int) { return it.operator++({}); }
        Iterator operator+(ptrdiff_t offset) { return it.operator+(offset); }
        Iterator operator-(ptrdiff_t offset) { return it.operator-(offset); }
        ptrdiff_t operator-(const Iterator& rhs) { return it.operator-(rhs.it); }
        const Token& operator*() const { return it.operator*(); }
        const Token* operator->() const { return it.operator->(); }
    };

    Iterator Peek() {
        return { tokens.begin() + index };
    }

    void Parse(sptr<ASTRoot>& root)
    {
        astRoot = root;
        scopes.Push(astRoot->global->scope.get());

        ParseSectionMembers(astRoot->global);
        ENFORCE(token.IsType(TokenType::EndOfFile), SourceLocation(), "Expected a definition or end of file");

        scopes.Pop();
        astRoot = nullptr;

        ENFORCE(token.type == TokenType::EndOfFile, token.loc, "Expected end of file");
    }

    void Parse(sptr<ASTRoot>& root, const ScopeStack& scopeStack)
    {
        astRoot = root;
        scopes = scopeStack;
        
        Scope* currentScope = scopes.GetCurrent();
        if(auto sect = currentScope->owner->ToSectionDefinition())
        {
            ParseSectionMembers(sect);
        }
        else if(auto classDef = currentScope->owner->ToClassDefinition())
        {
            ParseClassMembers(classDef);
        }
        else if(auto interfaceDef = currentScope->owner->ToInterfaceDefinition())
        {
            ParseInterfaceMembers(interfaceDef);
        }
        else if(auto structDef = currentScope->owner->ToStructDefinition())
        {
            ParseStructMembers(structDef);
        }
        else if(auto func = currentScope->owner->ToFunctionDefinition())
        {
            //ENFORCE(currentScope == func->body->scope.get(), SourceLocation(), "parse statement not valid here");

            while(!token.IsType(TokenType::EndOfFile))
                func->body->statements.push_back( ParseStatement() );
        }
        // TODO: should check for fold expressions, but I think they may just work via the function case
        
        astRoot = nullptr;

        ENFORCE(token.type == TokenType::EndOfFile, token.loc, "Expected end of file");
    }

private:
    void ParseSectionMembers(sptr<SectionDefinition>& section)
    {
        while (true)
        {
            if (token.IsKeyword(Keyword::Section))
                ParseSectionDefinition();
            else if (IsClassDefinition())
                ParseClassDefinition();
            else if (token.IsKeyword(Keyword::Interface))
                ParseInterfaceDefinition();
            else if (token.IsKeyword(Keyword::Struct))
                ParseStructDefinition();
            else if (token.IsKeyword(Keyword::Enum))
                ParseEnumDefinition();
            else if (token.IsKeyword(Keyword::Functor))
                ParseFunctorDefinition();
            else if (IsVariableDefinition())
                section->statements.push_back( ParseVarDefinitionStmt() );
            else if (IsFunctionDefinition())
                ParseFunctionDefinition();
            else
                break;
        }
    }

    sptr<SectionDefinition> ParseSectionDefinition()
    {
        const Token& sectionTok = Consume(Keyword::Section);

        const Token& nameTok = Consume(TokenType::Identifier);

        sptr<SectionDefinition> sect;

        sptr<Definition> def = scopes.GetCurrent()->FindDefinition(nameTok.GetIdentifier());
        if(def && def->ToSectionDefinition())
        {
            sect = def->ToSectionDefinition();
        }
        else
        {
            sect = spnew<SectionDefinition>(sectionTok.loc, scopes.GetCurrent(), nameTok.GetIdentifier());
            scopes.GetCurrent()->AddDefinition(sect);
        }
        
        Consume(TokenType::LeftBrace);
        scopes.Push(sect->scope.get());

        ParseSectionMembers(sect);
        ENFORCE(token.IsType(TokenType::RightBrace), token.loc, "Expected a definition or '}}'");

        scopes.Pop();
        Consume(TokenType::RightBrace);

        return sect;
    }

    void ParseClassMembers(sptr<ClassDefinition>& classDef)
    {
        while(true)
        {
            if (IsVariableDefinition())
            {
                auto varDef = ParseVariableDefinition();
                if(varDef->isStatic && !classDef->IsTemplateDeclaration())
                {
                    auto varDefStmt = spnew<VariableDefinitionStatement>(varDef->loc, astRoot->global->scope.get());
                    varDefStmt->variableDefinition = varDef;
                    astRoot->global->statements.push_back( varDefStmt );
                }
            }
            else if (IsPropertyDefinition())
            {
                ParsePropertyDefinition();
            }
            else if (IsFunctionDefinition())
            {
                ParseFunctionDefinition();
            }
            else
            {
                break;
            }
        }
    }

    void ParseExternClassMembers(sptr<ClassDefinition>& classDef)
    {
        assert(classDef->isExternal);

        while(true)
        {
            if(IsFunctionDefinition())
            {
                auto func = ParseFunctionDefinition();
                ENFORCE(func->isExternal, func->loc, "a function inside an extern class must also be extern");
            }
            else
            {
                break;
            }
        }
    }

    sptr<ClassDefinition> ParseClassDefinition()
    {
        bool isExternal = false;
        
        if(TryConsume(Keyword::Extern))
            isExternal = true;

        const Token& classTok = Consume(Keyword::Class);

        const Token& nameTok = Consume(TokenType::Identifier);

        auto def = spnew<ClassDefinition>(classTok.loc, scopes.GetCurrent(), nameTok.GetIdentifier());
        scopes.GetCurrent()->AddDefinition(def);

        def->isExternal = isExternal;

        if(isExternal)
        {
            ENFORCE(!token.IsType(TokenType::Less), token.loc, "extern classes cannot be templates");
            ENFORCE(!token.IsType(TokenType::Colon), token.loc, "extern classes cannot implement interfaces");

            if(!TryConsume(TokenType::Semicolon))
            {
                Consume(TokenType::LeftBrace);
                scopes.Push(def->scope.get());

                ParseExternClassMembers(def);
                ENFORCE(token.IsType(TokenType::RightBrace), token.loc, "Expected '}}' or extern function definition");

                scopes.Pop();
                Consume(TokenType::RightBrace);
            }
        }
        else
        {
            // parse template parameters
            if(TryConsume(TokenType::Less))
            {
                while(token.type != TokenType::Greater)
                {
                    const Token& tempParamNameTok = Consume(TokenType::Identifier);
                
                    auto typeSpec = spnew<TypeSpecifier>(tempParamNameTok.loc, def->scope.get(), shared_string("$placeholder"));
                    auto templateParamDef = spnew<TemplateParameterDefinition>(tempParamNameTok.loc, def->scope.get(), tempParamNameTok.GetIdentifier(), typeSpec);
                    def->scope->AddDefinition(templateParamDef);

                    if(!TryConsume(TokenType::Comma))
                        break;
                }

                Consume(TokenType::Greater);
            }

            // parse interfaces
            if(TryConsume(TokenType::Colon))
            {
                do {
                    auto interfaceTypeSpec = ParseTypeSpecifier();
                    def->interfaces.push_back(interfaceTypeSpec);
                } while(TryConsume(TokenType::Comma));
            }

            // consume class body
            Consume(TokenType::LeftBrace);
            scopes.Push(def->scope.get());

            ParseClassMembers(def);
            ENFORCE(token.IsType(TokenType::RightBrace), token.loc, "Expected '}}' or member definition");

            scopes.Pop();
            Consume(TokenType::RightBrace);
        }

        return def;
    }

    void ParseInterfaceMembers(sptr<InterfaceDefinition>& interfaceDef)
    {
        while(true)
        {
            if (IsFunctionDefinition())
            {
                ParseFunctionDefinition();
            }
            else
            {
                break;
            }
        }
    }

    sptr<InterfaceDefinition> ParseInterfaceDefinition()
    {
        const Token& classTok = Consume(Keyword::Interface);

        const Token& nameTok = Consume(TokenType::Identifier);

        auto def = spnew<InterfaceDefinition>(classTok.loc, scopes.GetCurrent(), nameTok.GetIdentifier());
        scopes.GetCurrent()->AddDefinition(def);

        // parse template parameters
        if(TryConsume(TokenType::Less))
        {
            while (token.type != TokenType::Greater)
            {
                const Token& tempParamNameTok = Consume(TokenType::Identifier);

                auto typeSpec = spnew<TypeSpecifier>(tempParamNameTok.loc, def->scope.get(), shared_string("$placeholder"));
                auto templateParamDef = spnew<TemplateParameterDefinition>(tempParamNameTok.loc, def->scope.get(), tempParamNameTok.GetIdentifier(), typeSpec);
                def->scope->AddDefinition(templateParamDef);

                if (!TryConsume(TokenType::Comma))
                    break;
            }

            Consume(TokenType::Greater);
        }

        // consume interface body
        Consume(TokenType::LeftBrace);
        scopes.Push(def->scope.get());

        ParseInterfaceMembers(def);
        ENFORCE(token.IsType(TokenType::RightBrace), token.loc, "Expected '}}' or function definition");

        scopes.Pop();
        Consume(TokenType::RightBrace);

        return def;
    }

    void ParseStructMembers(sptr<StructDefinition>& structDef)
    {
        while(true)
        {
            if (IsVariableDefinition())
            {
                auto varDef = ParseVariableDefinition();
                if(varDef->isStatic && !structDef->IsTemplateDeclaration())
                {
                    auto varDefStmt = spnew<VariableDefinitionStatement>(varDef->loc, astRoot->global->scope.get());
                    varDefStmt->variableDefinition = varDef;
                    astRoot->global->statements.push_back( varDefStmt );
                }
            }
            else if (IsPropertyDefinition())
            {
                ParsePropertyDefinition();
            }
            else if (IsFunctionDefinition())
            {
                ParseFunctionDefinition();
            }
            else
            {
                break;
            }
        }
    }

    sptr<StructDefinition> ParseStructDefinition()
    {
        const Token& classTok = Consume(Keyword::Struct);

        const Token& nameTok = Consume(TokenType::Identifier);

        auto def = spnew<StructDefinition>(classTok.loc, scopes.GetCurrent(), nameTok.GetIdentifier());
        scopes.GetCurrent()->AddDefinition(def);

        // parse template parameters
        if(TryConsume(TokenType::Less))
        {
            while (token.type != TokenType::Greater)
            {
                const Token& tempParamNameTok = Consume(TokenType::Identifier);

                auto typeSpec = spnew<TypeSpecifier>(tempParamNameTok.loc, def->scope.get(), shared_string("$placeholder"));
                auto templateParamDef = spnew<TemplateParameterDefinition>(tempParamNameTok.loc, def->scope.get(), tempParamNameTok.GetIdentifier(), typeSpec);
                def->scope->AddDefinition(templateParamDef);

                if (!TryConsume(TokenType::Comma))
                    break;
            }

            Consume(TokenType::Greater);
        }

        // consume class body
        Consume(TokenType::LeftBrace);
        scopes.Push(def->scope.get());

        ParseStructMembers(def);
        ENFORCE(token.IsType(TokenType::RightBrace), token.loc, "Expected '}}' or member definition");

        scopes.Pop();
        Consume(TokenType::RightBrace);

        return def;
    }

    sptr<EnumDefinition> ParseEnumDefinition()
    {
        const Token& classTok = Consume(Keyword::Enum);

        const Token& nameTok = Consume(TokenType::Identifier);

        auto def = spnew<EnumDefinition>(classTok.loc, scopes.GetCurrent(), nameTok.GetIdentifier());
        scopes.GetCurrent()->AddDefinition(def);

        Consume(TokenType::LeftBrace);
        scopes.Push(def->scope.get());

        std::optional<std::int64_t> previousValue;

        while(!token.IsType(TokenType::RightBrace))
        {
            const Token& memberTok = Consume(TokenType::Identifier);
            auto member = spnew<EnumMemberDefinition>(memberTok.loc, scopes.GetCurrent(), memberTok.GetIdentifier());
            scopes.GetCurrent()->AddDefinition(member);

            if(TryConsume(TokenType::Assign))
            {
                auto valueExpr = ParseIntegerLiteral();
                member->value = valueExpr;
                previousValue = valueExpr->value;
            }
            else
            {
                int64_t value = previousValue ? *previousValue + 1 : 0;
                member->value = spnew<IntegerLiteralExpression>(member->loc, scopes.GetCurrent(), value);
                previousValue = value;
            }
            
            if(!TryConsume(TokenType::Comma))
                break;
        }

        scopes.Pop();
        Consume(TokenType::RightBrace);

        return def;
    }

    bool ScanTypeSpecifier(Iterator& it)
    {
        auto start = it;

        if(!it->IsType(TokenType::Identifier) || (it->IsKeyword() && !it->IsBasicType()))
            goto Fail; // expected identifier

        ++it; // identifier

        while(it->IsType(TokenType::Dot))
        {
            ++it; // "."

            if(!it->IsType(TokenType::Identifier) || (it->IsKeyword() && !it->IsBasicType()))
                goto Fail; // expected identifier after "."

            ++it; // identifier
        }

        // template arguments
        if(it->IsType(TokenType::Less))
        {
            ++it; // "<"

            while(!it->IsType(TokenType::Greater))
            {
                if(!ScanTypeSpecifier(it))
                    goto Fail; // expected template argument

                if (!it->IsType(TokenType::Comma))
                    break;

                ++it; // ,
            }

            ++it; // ">"
        }

        // optional array brackets
        while(it->IsType(TokenType::LeftBracket) && (it + 1)->IsType(TokenType::RightBracket))
        {
            ++it; // [
            ++it; // ]
        }

    // Pass
        return true;

    Fail:
        it = start;
        return false;
    }

    bool IsGreater()
    {
        auto it = Peek();
        if(!it->IsType(TokenType::Greater))
            return false;

        ++it;
        if(it->IsType(TokenType::Greater) || it->IsType(TokenType::Assign))
            return false;

        return true;
    }

    bool IsGreaterEqual()
    {
        auto it = Peek();
        if(!it->IsType(TokenType::Greater))
            return false;

        ++it;

        return it->IsType(TokenType::Assign);
    }

    bool IsRightShift()
    {
        auto it = Peek();
        if(!it->IsType(TokenType::Greater))
            return false;

        ++it;

        if(!it->IsType(TokenType::Greater))
            return false;

        ++it;

        return !it->IsType(TokenType::Assign);
    }

    bool IsRightShiftAssign()
    {
        auto it = Peek();
        if(!it->IsType(TokenType::Greater))
            return false;

        ++it;
        if(!it->IsType(TokenType::Greater))
            return false;

        ++it;
        return it->IsType(TokenType::Assign);
    }
    
    bool IsClassDefinition()
    {
        auto it = Peek();

        if(it->IsKeyword(Keyword::Extern))
            ++it;

        if(!it->IsKeyword(Keyword::Class))
            return false;

        ++it;

        // class name
        if(it->type != TokenType::Identifier || it->IsKeyword())
            return false;

        return true;
    }

    bool IsVariableDefinition()
    {
        auto it = Peek();

        // variable storage class
        while(true)
        {
            if(it->IsKeyword(Keyword::Static))
                ++it;
            else if(it->IsKeyword(Keyword::Private))
                ++it;
            else
                break;
        }

        // variable type
        if(!ScanTypeSpecifier(it))
            return false;

        // variable name
        if(it->type != TokenType::Identifier || it->IsKeyword())
            return false;

        ++it;

        // semicolon or initializer
        if(it->type != TokenType::Semicolon && it->type != TokenType::Assign)
            return false;

        // found a variable definition
        return true;
    }

    bool IsPropertyDefinition()
    {
        auto it = Peek();

        // optional storage class
        while(true)
        {
            if(it->IsKeyword(Keyword::Static))
                ++it;
            else if(it->IsKeyword(Keyword::Private))
                ++it;
            else
                break;
        }

        // property type
        if(!ScanTypeSpecifier(it))
            return false;

        // property name
        if(it->type != TokenType::Identifier || it->IsKeyword())
            return false;

        ++it;

        // opening brace
        if(it->type != TokenType::LeftBrace)
            return false;

        // found a variable definition
        return true;
    }

    bool IsFunctionDefinition()
    {
        auto it = Peek();

        // optional leading storage classes
        while(true)
        {
            if(it->IsKeyword(Keyword::Extern))
                ++it;
            else if(it->IsKeyword(Keyword::Static))
                ++it;
            else if(it->IsKeyword(Keyword::Private))
                ++it;
            else
                break;
        }

        if(it->IsKeyword(Keyword::This))
        {
            ++it;
        }
        else
        {
            // return type
            if(!ScanTypeSpecifier(it))
                return false;

            // function name
            if(it->IsKeyword(Keyword::Operator))
            {
                ++it;

                if(it->IsType(TokenType::LeftBracket))
                {
                    ++it;

                    if(it->IsType(TokenType::RightBracket))
                    {
                        ++it;
                    }
                    else
                    {
                        // incomplete operator[]
                        return false;
                    }
                }
                else if(OverloadableBinaryOperators.contains(it->type))
                {
                    ++it;
                }
                else if(OverloadablePrefixOperators.contains(it->type))
                {
                    ++it;
                }
                else if(OverloadablePostfixOperators.contains(it->type))
                {
                    ++it;
                }
                else
                {
                    // unrecognized operator overload
                    return false;
                }
            }
            else if(it->IsType(TokenType::Identifier) && !it->IsKeyword())
            {
                ++it;
            }
            else
            {
                // invalid function name
                return false;
            }
        }

        // opening paren
        if(it->type != TokenType::LeftParen)
            return false;

        // found a function
        return true;
    }

    bool IsTypeSpecifier()
    {
        auto it = Peek();
        return ScanTypeSpecifier(it);
    }

    sptr<TypeSpecifier> ParseTypeSpecifier()
    {
        std::string baseTypeName;
        int arrayDimensions = 0;

        auto loc = token.loc;

        shared_string id = Consume(TokenType::Identifier).GetIdentifier();
        baseTypeName.append(id);

        while(TryConsume(TokenType::Dot))
        {
            id = Consume(TokenType::Identifier).GetIdentifier();

            if(!baseTypeName.empty())
                baseTypeName.append(".");
            
            baseTypeName.append(id);
        }

        // parse template arguments
        std::vector<sptr<TypeSpecifier>> templateArgs;

        if(TryConsume(TokenType::Less))
        {
            while(token.type != TokenType::Greater)
            {
                auto templateArg = ParseTypeSpecifier();
                templateArgs.push_back(templateArg);

                if (!TryConsume(TokenType::Comma))
                    break;
            }

            Consume(TokenType::Greater);
        }

        auto it = Peek();
        while((it++)->IsType(TokenType::LeftBracket) && (it++)->IsType(TokenType::RightBracket))
        {
            Consume(TokenType::LeftBracket);
            Consume(TokenType::RightBracket);
            ++arrayDimensions;
        }

        return spnew<TypeSpecifier>(loc, scopes.GetCurrent(), shared_string(std::move(baseTypeName)), templateArgs, arrayDimensions);
    }

    sptr<InterfaceDefinition> ParseFunctorDefinition()
    {
        const Token& functorTok = Consume(Keyword::Functor);
        
        sptr<TypeSpecifier> returnType = ParseTypeSpecifier();
        shared_string functorName = Consume(TokenType::Identifier).GetIdentifier();

        Consume(TokenType::LeftParen);

        std::string paramList;

        while (token.type != TokenType::RightParen)
        {
            auto typeSpec = ParseTypeSpecifier();
            const Token& nameTok = Consume(TokenType::Identifier);
            auto paramType = typeSpec->GetTypeName(true);
            auto paramName = nameTok.GetIdentifier();

            // +2 for space and comma
            paramList.reserve(paramList.size() + paramType.size() + paramName.size() + 2);

            if(!paramList.empty())
                paramList += ", ";

            paramList += paramType.view();
            paramList += " ";
            paramList += paramName;

            if (!TryConsume(TokenType::Comma))
                break;
        }

        Consume(TokenType::RightParen);
        Consume(TokenType::Semicolon);

        constexpr const char* mixinFormat = R""""(
interface {}
{{
    {} invoke({});
}}
)"""";

        std::string mixinCode = std::format(mixinFormat,
            functorName,
            returnType->GetTypeName(true).view(),
            paramList
        );

        ParseCodeString(mixinCode, functorTok.loc.line);

        auto def = scopes.GetCurrent()->FindDefinition(functorName);
        assert(def);

        auto functorInterface = def->ToInterfaceDefinition();
        assert(functorInterface);
        
        functorInterface->isFunctor = true;

        return functorInterface;
    }

    Scope* ParseCodeString(const std::string& code, size_t lineNumber, bool forceSectionScope = false)
    {
        Lexer lexer(token.loc.file.view(), code, lineNumber, true);
        std::vector<Token> tokens = lexer.Tokenize();

        std::vector<Scope*> poppedScopes;

        if(forceSectionScope)
        {
            while(!scopes.GetCurrent()->owner->ToSectionDefinition())
            {
                poppedScopes.push_back(scopes.GetCurrent());
                scopes.Pop();
            }
        }

        Scope* scopeOfParse = scopes.GetCurrent();

        Parser parser(tokens);
        parser.Parse(astRoot, scopes);

        if(forceSectionScope)
        {
            while(!poppedScopes.empty())
            {
                scopes.Push(poppedScopes.back());
                poppedScopes.pop_back();
            }
        }

        return scopeOfParse;
    }

    sptr<FunctionDefinition> ParseFunctionDefinition(const std::string& overrideName = {})
    {
        auto loc = token.loc;
        auto owner = scopes.GetCurrent()->owner;

        bool isExternal = false;
        bool isAbstract = false;
        bool isStatic = false;
        bool isDeclaredStatic = false;
        bool isPrivate = false;
        bool isMember = false;
        bool isConstructor = false;

        while(true)
        {
            if(TryConsume(Keyword::Extern)) {
                isExternal = true;
                isStatic = true;
            }
            else if(TryConsume(Keyword::Static)) {
                isDeclaredStatic = true;
                isStatic = true;
            }
            else if(TryConsume(Keyword::Private)) {
                isPrivate = true;
            }
            else {
                break;
            }
        }

        sptr<TypeSpecifier> returnType;
        std::string name;

        if(token.IsKeyword(Keyword::This))
        {
            assert(overrideName.empty());
            ENFORCE(owner->ToClassDefinition() || owner->ToStructDefinition(), token.loc,
                "constructors are only valid inside a class or struct");

            returnType = spnew<TypeSpecifier>(token.loc, scopes.GetCurrent(), owner->name);

            auto templateParams = owner->GetChildren<TemplateParameterDefinition>();
            if(!templateParams.empty())
            {
                for(const auto& param : templateParams)
                {
                    auto arg = spnew<TypeSpecifier>(token.loc, scopes.GetCurrent(), param->name);
                    returnType->templateArgs.push_back(arg);
                }
            }

            name = "#this";
            Consume(Keyword::This);
            isConstructor = true;
        }
        else
        {
            returnType = ParseTypeSpecifier();

            if(!overrideName.empty())
                name = overrideName;
            else
                name = Consume(TokenType::Identifier).GetIdentifier();
        }

        if(name == "operator")
        {
            if(TryConsume(TokenType::LeftBracket))
            {
                Consume(TokenType::RightBracket);
                name += "[]";
            }
            else if(OverloadableBinaryOperators.contains(token.type))
            {
                name += TokenNames.at(token.type);
                Consume();
            }
            else if(OverloadablePrefixOperators.contains(token.type))
            {
                name += TokenNames.at(token.type);
                Consume();
            }
            else if(OverloadablePostfixOperators.contains(token.type))
            {
                name += TokenNames.at(token.type);
                Consume();
            }
            else
            {
                ENFORCE(false, token.loc, "expected operator");
            }
        }
        
        bool isOwnerExternClass = false;

        if(auto ownerClassDef = owner->ToClassDefinition())
        {
            isMember = true;
            isOwnerExternClass = ownerClassDef->isExternal;
        }
        else if(owner->ToStructDefinition() != nullptr)
        {
            isMember = true;
        }
        else if(owner->ToInterfaceDefinition() != nullptr)
        {
            isMember = true;
            isAbstract = true;
        }
        else
        {
            isStatic = true;
        }

        bool isTask = (returnType->baseTypeName == "Task");
        ENFORCE(!isConstructor || !isStatic || isExternal, loc, "invalid storage class for constructor");

        auto def = spnew<FunctionDefinition>(loc, scopes.GetCurrent(), shared_string(name));
        def->isExternal = isExternal;
        def->isAbstract = isAbstract;
        def->isStatic = isStatic;
        def->isPrivate = isPrivate;
        def->isMember = isMember;
        def->isConstructor = isConstructor;
        def->returnType = returnType;
        scopes.GetCurrent()->AddDefinition(def);

        auto& leftParenTok = Consume(TokenType::LeftParen);
        scopes.Push(def->scope.get());

        // if function is a non-static extern member function, add context as first parameter
        if(isOwnerExternClass && isExternal && !isDeclaredStatic && name != "#this")
        {
            auto contextTypeSpec = spnew<TypeSpecifier>(leftParenTok.loc, scopes.GetCurrent(), owner->qualifiedName);
            auto contextParam = spnew<ParameterDefinition>(leftParenTok.loc, scopes.GetCurrent(), contextTypeSpec, shared_string("$this"));
            scopes.GetCurrent()->AddDefinition(contextParam);
            def->isUFC = true;
        }

        while (token.type != TokenType::RightParen)
        {
            ParseParameterDefinition();

            if (!TryConsume(TokenType::Comma))
                break;
        }
        
        Consume(TokenType::RightParen);
        
        if(isTask)
        {
            auto bodyLoc = token.loc;
            auto externFuncLoc = scopes.GetCurrent()->owner->loc;

            scopes.Pop(); // temporarily pop function scope to parse task functor and modified extern func

            if(isExternal)
            {
                // original function turns into wrapper that returns the task
                def->isExternal = false;

                std::string taskParam { returnType->GetTypeName(true) };
                taskParam += " t";

                std::string paramList;
                for(const auto& param : def->GetChildren<ParameterDefinition>())
                {
                    if(!paramList.empty())
                        paramList += ", ";

                    paramList += param->typeSpec->GetTypeName(true) + " " + param->name;
                }
                std::string mixinCode = std::format("extern void ${}({}, {});", name, taskParam, paramList);
                ParseCodeString(mixinCode, externFuncLoc.line);
            }

            auto task = ParseTaskObject(def, isExternal);
            scopes.Push(def->scope.get()); // re-push function scope

            std::string argList;
            std::string paramAssignments;

            if(isExternal)
            {
                for(const auto& param : def->GetChildren<ParameterDefinition>())
                {
                    if(!argList.empty())
                        argList += ", ";

                    argList += param->name;
                }
            }
            else
            {
                for(const auto& param : def->GetChildren<ParameterDefinition>())
                {
                    //$ret.param = param;
                    std::string paramTypeName { param->typeSpec->GetTypeName(true) };
                    paramAssignments += "$ret." + param->name + " = " + param->name + ";\n    ";
                }
            }

            // create function body
            constexpr const char* mixinFormat = R""""(
{} $ret = new {}{{}};

// non-external
{}$ret.$this = this;
{}
{}$ret.Resume();
    
// external
{}${}($ret, {});

return $ret;
)"""";

            const char* comment = "// ";

            std::string mixinCode = std::format(mixinFormat,
                task->name, // $ret instantiation
                task->name,
                !isExternal && !def->isStatic ? "" : comment, // $this assignment
                paramAssignments,
                !isExternal ? "" : comment, // Resume call
                isExternal ? "" : comment, // external function call
                name,
                argList
            );

            def->body = spnew<BlockStatement>(bodyLoc, scopes.GetCurrent());
            scopes.Push(def->body->scope.get());

            ParseCodeString(mixinCode, bodyLoc.line);

            scopes.Pop();

            // function scope popped below...
        }
        else if(def->isExternal || def->isAbstract)
        {
            Consume(TokenType::Semicolon);
        }
        else
        {
            def->body = spnew<BlockStatement>(token.loc, scopes.GetCurrent());
            ParseBlockStmt(def->body);

            if(def->returnType->IsVoid())
            {
                if(def->body->statements.empty() || !def->body->statements.back()->ToReturnStatement())
                {
                    auto loc = def->body->statements.empty() ?
                        def->body->loc : def->body->statements.back()->loc;

                    auto ret = spnew<ReturnStatement>(loc, scopes.GetCurrent());
                    def->body->statements.push_back(ret);
                }
            }
            else if(isConstructor)
            {
                auto loc = def->body->statements.empty() ?
                    def->body->loc : def->body->statements.back()->loc;

                auto val = spnew<IdentifierExpression>(loc, scopes.GetCurrent(), shared_string("this"));
                auto ret = spnew<ReturnStatement>(loc, scopes.GetCurrent(), val);
                def->body->statements.push_back(ret);
            }
        }

        scopes.Pop();

        return def;
    }
    
    sptr<ClassDefinition> ParseTaskObject(const sptr<FunctionDefinition>& func, bool isExternal)
    {
        auto& yieldType = func->returnType->templateArgs.back();

        auto yieldTypeName = yieldType->GetTypeName();
        auto yieldTypeIsVoid = yieldType->IsVoid();
        std::string instanceTypeName;
        std::string paramList;

        auto enclosingClass = func->parent->ToClassDefinition();

        if(!isExternal)
        {
            if(enclosingClass && !func->isStatic)
            {
                instanceTypeName = enclosingClass->name;
            }

            // params
            for(const auto& param : func->GetChildren<ParameterDefinition>())
            {
                paramList += param->typeSpec->GetTypeName(true) + " " + param->name + ";\n    ";
            }
        }

        constexpr const char* mixinFormat = R""""(
// isCoroutineState set after
class ${}_Task
    : Task<{}>
    , Awaitable
    {}, Awaiter
{{
    int $position = 0;

    Awaiter $awaiter = null;
    Awaitable $awaited = null;
    {}{} $value;
    {}{} $this;

    // parameters
    {}
    
    // isCoroutine set after
    void Resume()
    {{
        {}if($position != 0) goto $position;

        // body parsed below...
    }}

    {} GetValue()
    {{
        {}return $value;
    }}

    bool IsDone()
    {{
        return $position == -1;
    }}

    void SetAwaiter(Awaiter parentTask)
    {{
        $awaiter = parentTask;
    }}
 
    void ResumeAwaiter()
    {{
        if($awaiter != null)
            $awaiter.Resume();
    }}
}}
)"""";

        const char* comment = "// ";

        std::string mixinCode = std::format(mixinFormat,
            func->name, //  {}_Task
            yieldTypeName, //  Task<{}>
            !isExternal ? "" : comment, //  {}, Awaiter
            !yieldTypeIsVoid ? "" : comment, //  {}{} $value;
            yieldTypeName, // ...
            !instanceTypeName.empty() ? "" : comment, //  {}{} $this;
            !instanceTypeName.empty() ? instanceTypeName : "void", // ...
            paramList, //  {}
            !isExternal ? "" : comment, //  {}if($position != 0) goto $position;
            yieldTypeName, //  {} GetValue()
            !yieldTypeIsVoid ? "" : comment //  {}return $value;
        );

        Scope* scopeOfParse = ParseCodeString(mixinCode, func->parent->loc.line, true);

        auto taskObjectName = std::format("${}_Task", func->name);

        auto taskObject = scopeOfParse->FindDefinition(taskObjectName)->ToClassDefinition();
        taskObject->isCoroutineState = true;
        
        if(enclosingClass)
        {
            taskObject->originalClassType = spnew<TypeSpecifier>(enclosingClass->loc, enclosingClass->enclosingScope, enclosingClass->name);
        }
        
        if(isExternal)
        {
            Consume(TokenType::Semicolon);
        }
        else
        {
            auto resumeFunc = taskObject->GetFunction("Resume");
            resumeFunc->isCoroutine = true;

            // since return works differently for coroutines, return type is always void, and implicit return is always added
            auto implicitReturn = resumeFunc->body->statements.back()->ToReturnStatement();
            assert(implicitReturn && !implicitReturn->expression);
            resumeFunc->body->statements.pop_back();

            scopes.Push(taskObject->scope.get());
            scopes.Push(resumeFunc->scope.get());
            ParseBlockStmt(resumeFunc->body);

            if(yieldType->IsVoid())
            {
                if(resumeFunc->body->statements.empty() || !resumeFunc->body->statements.back()->ToReturnStatement())
                {
                    auto loc = resumeFunc->body->statements.empty() ?
                        resumeFunc->body->loc : resumeFunc->body->statements.back()->loc;

                    auto ret = spnew<ReturnStatement>(loc, scopes.GetCurrent());
                    resumeFunc->body->statements.push_back(ret);
                }
            }

            scopes.Pop();
            scopes.Pop();
        }

        //ASTPrinter::Print(taskObject);

        return taskObject;
    }

    sptr<ParameterDefinition> ParseParameterDefinition()
    {
        auto typeSpec = ParseTypeSpecifier();

        const Token& nameTok = Consume(TokenType::Identifier);

        auto def = spnew<ParameterDefinition>(nameTok.loc, scopes.GetCurrent(), typeSpec, nameTok.GetIdentifier());
        scopes.GetCurrent()->AddDefinition(def);

        return def;
    }

    sptr<VariableDefinition> ParseVariableDefinition()
    {
        auto owner = scopes.GetCurrent()->owner;

        bool isStatic = false;
        bool isPrivate = false;

        while(true)
        {
            auto loc = token.loc;

            if(TryConsume(Keyword::Static)) {
                ENFORCE(!owner->ToSectionDefinition(), loc, "'static' not valid in a section");
                isStatic = true;
            }
            else if(TryConsume(Keyword::Private)) {
                isPrivate = true;
            }
            else {
                break;
            }
        }

        auto typeSpec = ParseTypeSpecifier();
        
        const Token& nameTok = Consume(TokenType::Identifier);

        sptr<VariableDefinition> def = spnew<VariableDefinition>(nameTok.loc, scopes.GetCurrent(), typeSpec, nameTok.GetIdentifier());
        scopes.GetCurrent()->AddDefinition(def);

        def->isStatic = owner->ToSectionDefinition() ? true : isStatic;
        def->isPrivate = isPrivate;

        if (TryConsume(TokenType::Assign))
        {
            def->initializer = ParseExpression();
        }
        else
        {
            auto defaultTypeSpec = spnew<TypeSpecifier>(*typeSpec);
            def->initializer = spnew<DefaultValueExpression>(token.loc, scopes.GetCurrent(), defaultTypeSpec);
        }

        Consume(TokenType::Semicolon);

        return def;
    }

    sptr<PropertyDefinition> ParsePropertyDefinition()
    {
        bool isStatic = false;
        bool isPrivate = false;

        while(true)
        {
            if(TryConsume(Keyword::Static)) {
                isStatic = true;
            }
            else if(TryConsume(Keyword::Private)) {
                isPrivate = true;
            }
            else {
                break;
            }
        }

        auto propertyTypeSpec = ParseTypeSpecifier();

        const Token& nameTok = Consume(TokenType::Identifier);
        shared_string propertyName = nameTok.GetIdentifier();

        sptr<PropertyDefinition> def = spnew<PropertyDefinition>(nameTok.loc, scopes.GetCurrent(), propertyTypeSpec, propertyName);
        scopes.GetCurrent()->AddDefinition(def);

        def->isStatic = isStatic;
        def->isPrivate = isPrivate;

        Consume(TokenType::LeftBrace);

        while(!TryConsume(TokenType::RightBrace))
        {
            if(token.IsKeyword(Keyword::Get))
            {
                const Token& getTok = Consume(Keyword::Get);

                auto getterFunc = spnew<FunctionDefinition>(getTok.loc, scopes.GetCurrent(), shared_string("$get_" + propertyName));
                getterFunc->isStatic = isStatic;
                getterFunc->isMember = true;
                getterFunc->returnType = spnew<TypeSpecifier>(*propertyTypeSpec);
                scopes.GetCurrent()->AddDefinition(getterFunc);
                
                scopes.Push(getterFunc->scope.get());

                getterFunc->body = spnew<BlockStatement>(token.loc, scopes.GetCurrent());
                ParseBlockStmt(getterFunc->body);

                scopes.Pop();

                def->getterName = getterFunc->name;
            }
            else if(token.IsKeyword(Keyword::Set))
            {
                const Token& setTok = Consume(Keyword::Set);

                auto setterFunc = spnew<FunctionDefinition>(setTok.loc, scopes.GetCurrent(), shared_string("$set_" + propertyName));
                setterFunc->isStatic = isStatic;
                setterFunc->isMember = true;
                setterFunc->returnType = spnew<TypeSpecifier>(setTok.loc, scopes.GetCurrent(), shared_string("void"));
                scopes.GetCurrent()->AddDefinition(setterFunc);

                // enter function param scope
                scopes.Push(setterFunc->scope.get());

                // add value param
                auto argTypeSpec = spnew<TypeSpecifier>(*propertyTypeSpec);
                auto paramDef = spnew<ParameterDefinition>(setTok.loc, scopes.GetCurrent(), argTypeSpec, shared_string("value"));
                scopes.GetCurrent()->AddDefinition(paramDef);

                // parse function body
                setterFunc->body = spnew<BlockStatement>(token.loc, scopes.GetCurrent());
                ParseBlockStmt(setterFunc->body);

                // add implicit return statement if needed
                if(setterFunc->body->statements.empty() || !setterFunc->body->statements.back()->ToReturnStatement())
                {
                    auto loc = setterFunc->body->statements.empty() ?
                        setterFunc->body->loc : setterFunc->body->statements.back()->loc;

                    auto ret = spnew<ReturnStatement>(loc, scopes.GetCurrent());
                    setterFunc->body->statements.push_back(ret);
                }

                // exit function param scope
                scopes.Pop();

                def->setterName = setterFunc->name;
            }
        }

        //auto defaultTypeSpec = spnew<TypeSpecifier>(*typeSpec);
        //def->initializer = spnew<DefaultValueExpression>(token.loc, scopes.GetCurrent(), defaultTypeSpec);

        return def;
    }

    sptr<BlockStatement> ParseBlockStmt(sptr<BlockStatement> body = {})
    {
        auto block = body ? body : spnew<BlockStatement>(token.loc, scopes.GetCurrent());

        scopes.Push(block->scope.get());
        Consume(TokenType::LeftBrace);
        
        while (token.type != TokenType::RightBrace)
            block->statements.push_back( ParseStatement() );

        Consume(TokenType::RightBrace);
        scopes.Pop();
        return block;
    }

    sptr<Statement> ParseStatement()
    {
        sptr<Statement> stmt;

        if (token.type == TokenType::LeftBrace)
            stmt = ParseBlockStmt();
        else if (token.IsKeyword(Keyword::If))
            stmt = ParseIfStmt();
        else if (token.IsKeyword(Keyword::For))
            stmt = ParseForStmt();
        else if (token.IsKeyword(Keyword::While))
            stmt = ParseWhileStmt();
        else if (token.IsKeyword(Keyword::Return))
            stmt = ParseReturnStmt();
        else if (token.IsKeyword(Keyword::Goto))
            stmt = ParseGotoStmt();
        else if (token.IsKeyword(Keyword::Assert))
            stmt = ParseAssertStmt();
        else if (token.IsKeyword(Keyword::Code))
            stmt = ParseCodeStmt();
        else if (IsVariableDefinition())
            stmt = ParseVarDefinitionStmt();
        else
            stmt = ParseExpressionStmt();

        return stmt;
    }
    
    sptr<Statement> ParseVarDefinitionStmt()
    {
        sptr<Statement> stmt;

        auto scope = scopes.GetCurrent();
        if(scope->owner->parent &&
            scope->owner->parent->ToClassDefinition() &&
            scope->owner->parent->ToClassDefinition()->isCoroutineState)
        {
            stmt = ParseCoroutineFieldStmt(scope->owner->parent->ToClassDefinition());
        }
        else
        {
            auto varDefStmt = spnew<VariableDefinitionStatement>(token.loc, scopes.GetCurrent());
            varDefStmt->variableDefinition = ParseVariableDefinition();
            stmt = varDefStmt;
        }

        return stmt;
    }

    sptr<Statement> ParseCoroutineFieldStmt(const sptr<ClassDefinition>& coroutine)
    {
        auto typeSpec = ParseTypeSpecifier();

        const Token& nameTok = Consume(TokenType::Identifier);

        Scope* coroutineScope = coroutine->scope.get();

        sptr<VariableDefinition> field = spnew<VariableDefinition>(nameTok.loc, coroutineScope, typeSpec, nameTok.GetIdentifier());
        coroutineScope->AddDefinition(field);

        auto left = spnew<IdentifierExpression>(token.loc, scopes.GetCurrent(), field->name);
        sptr<Expression> right;

        if (TryConsume(TokenType::Assign))
        {
            right = ParseExpression();
        }
        else
        {
            ENFORCE(typeSpec->baseTypeName != "var", typeSpec->loc, "var must have an initializer");
            auto defaultTypeSpec2 = spnew<TypeSpecifier>(field->loc, coroutineScope, typeSpec->baseTypeName, typeSpec->templateArgs, typeSpec->arrayDimensions);
            right = spnew<DefaultValueExpression>(token.loc, coroutineScope, defaultTypeSpec2);
        }

        Consume(TokenType::Semicolon);

        auto fieldInitExpr = spnew<AssignExpression>(left->loc, scopes.GetCurrent(), TokenType::Assign, left, right);
        fieldInitExpr->fieldToInitialize = field.get();

        auto stmt = spnew<ExpressionStatement>(fieldInitExpr, scopes.GetCurrent());

        if(typeSpec->baseTypeName == "var")
        {
            // Default value will be inferred from the right side of the field-init assignment expression
            // that replaced the variable definition that was in the body of the function.
            field->initializer = spnew<DefaultValueExpression>(field->loc, fieldInitExpr->scope, fieldInitExpr);
        }
        else
        {
            auto declaredTypeSpec = spnew<TypeSpecifier>(field->loc, coroutineScope, typeSpec->baseTypeName, typeSpec->templateArgs, typeSpec->arrayDimensions);
            field->initializer = spnew<DefaultValueExpression>(field->loc, coroutineScope, declaredTypeSpec);
        }

        return stmt;
    }

    sptr<IfStatement> ParseIfStmt()
    {
        auto stmt = spnew<IfStatement>(token.loc, scopes.GetCurrent());
        scopes.Push(stmt->scope.get());

        Consume(Keyword::If);
        Consume(TokenType::LeftParen);
        stmt->condition = ParseExpression();
        Consume(TokenType::RightParen);

        if(token.type == TokenType::LeftBrace)
            stmt->trueBranch = ParseBlockStmt();
        else
            stmt->trueBranch = ParseStatement();

        if(TryConsume(Keyword::Else))
        {
            if(token.type == TokenType::LeftBrace)
                stmt->falseBranch = ParseBlockStmt();
            else
                stmt->falseBranch = ParseStatement();
        }

        scopes.Pop();
        return stmt;
    }

    sptr<ForStatement> ParseForStmt()
    {
        auto stmt = spnew<ForStatement>(token.loc, scopes.GetCurrent());
        scopes.Push(stmt->scope.get());

        Consume(Keyword::For);
        Consume(TokenType::LeftParen);

        if (IsVariableDefinition())
            stmt->init = ParseVarDefinitionStmt();
        else if(token.type == TokenType::Semicolon)
            Consume(TokenType::Semicolon);
        else
            stmt->init = ParseExpressionStmt();

        if(token.type != TokenType::Semicolon)
            stmt->condition = ParseExpression();

        Consume(TokenType::Semicolon);
        
        stmt->iterate = ParseExpressionStmt(false);
        
        Consume(TokenType::RightParen);

        stmt->body = ParseStatement();

        scopes.Pop();
        return stmt;
    }

    sptr<WhileStatement> ParseWhileStmt()
    {
        auto stmt = spnew<WhileStatement>(token.loc, scopes.GetCurrent());
        scopes.Push(stmt->scope.get());

        Consume(Keyword::While);
        Consume(TokenType::LeftParen);
        stmt->condition = ParseExpression();
        Consume(TokenType::RightParen);

        stmt->body = ParseStatement();

        scopes.Pop();
        return stmt;
    }

    sptr<Statement> ParseReturnStmt()
    {
        sptr<Statement> stmt;
        sptr<Expression> expr;

        Consume(Keyword::Return);

        if(!token.IsType(TokenType::Semicolon))
            expr = ParseExpression();

        Consume(TokenType::Semicolon);

        stmt = spnew<ReturnStatement>(token.loc, scopes.GetCurrent(), expr);

        return stmt;
    }

    sptr<Statement> ParseGotoStmt()
    {
        auto stmt = spnew<GotoStatement>(token.loc, scopes.GetCurrent());

        Consume(Keyword::Goto);
        stmt->expression = ParseExpression();
        Consume(TokenType::Semicolon);

        return stmt;
    }
    
    sptr<Statement> ParseAssertStmt()
    {
        const Token& tok = Consume(Keyword::Assert);
        Consume(TokenType::LeftParen);

        sptr<Expression> condition = ParseExpression();
        sptr<Expression> message;

        if (TryConsume(TokenType::Comma))
            message = ParseExpression();

        Consume(TokenType::RightParen);
        Consume(TokenType::Semicolon);

        return spnew<AssertStatement>(tok.loc, scopes.GetCurrent(), condition, message);
    }
    
    sptr<Statement> ParseCodeStmt()
    {
        const Token& tok = Consume(Keyword::Code);
        Consume(TokenType::LeftParen);
        shared_string mixinCode = Consume(TokenType::StringLiteral).GetString();
        Consume(TokenType::RightParen);
        Consume(TokenType::Semicolon);

        Lexer lexer(tok.loc.file.view(), mixinCode.str(), tok.loc.line, false);
        std::vector<Token> tokens = lexer.Tokenize();

        Parser parser(tokens);
        parser.Parse(astRoot, scopes);

        // return empty expr since code was added by parser
        auto expr = spnew<NullLiteralExpression>(tok.loc, scopes.GetCurrent());
        auto stmt = spnew<ExpressionStatement>(expr, scopes.GetCurrent());

        return stmt;
    }

    sptr<ExpressionStatement> ParseExpressionStmt(bool parseSemicolon = true)
    {
        auto stmt = spnew<ExpressionStatement>(ParseExpression(), scopes.GetCurrent());

        if(parseSemicolon)
            Consume(TokenType::Semicolon);

        return stmt;
    }

    sptr<Expression> ParseExpression()
    {
        return ParseAssignment();
    }

    sptr<Expression> ParseAssignment()
    {
        sptr<Expression> expr = ParseTernaryExpr();

        while(IsAnyOf(token.type,
            TokenType::Assign,
            TokenType::AddAssign,
            TokenType::SubAssign,
            TokenType::MulAssign,
            TokenType::DivAssign,
            TokenType::ModAssign,
            TokenType::LeftShiftAssign,
            TokenType::BitAndAssign,
            TokenType::BitOrAssign,
            TokenType::BitXorAssign) || IsRightShiftAssign())
        {
            SourceLocation loc;
            TokenType type;

            if(IsRightShiftAssign())
            {
                loc = token.loc;
                type = TokenType::RightShiftAssign;
                Consume(TokenType::Greater);
                Consume(TokenType::Greater);
                Consume(TokenType::Assign);
            }
            else
            {
                loc = token.loc;
                type = token.type;
                Consume();
            }

            sptr<Expression> right = ParseAssignment();
            expr = spnew<AssignExpression>(loc, scopes.GetCurrent(), type, expr, right);
        }

        return expr;
    }

    sptr<Expression> ParseTernaryExpr()
    {
        sptr<Expression> expr = ParseLogicalOrExpression();

        if(token.IsType(TokenType::QuestionMark))
        {
            const Token& oper = Consume(TokenType::QuestionMark);
            sptr<Expression> trueExpr = ParseExpression();
            Consume(TokenType::Colon);
            sptr<Expression> falseExpr = ParseTernaryExpr();
            expr = spnew<TernaryExpression>(expr->loc, scopes.GetCurrent(), expr, trueExpr, falseExpr);
        }

        return expr;
    }

    sptr<Expression> ParseLogicalOrExpression()
    {
        sptr<Expression> expr = ParseLogicalAndExpression();

        while(token.type == TokenType::LogicalOr)
        {
            const Token& oper = Consume();
            sptr<Expression> right = ParseLogicalAndExpression();
            expr = spnew<BinaryExpression>(oper.loc, scopes.GetCurrent(), oper.type, expr, right);
        }

        return expr;
    }

    sptr<Expression> ParseLogicalAndExpression()
    {
        sptr<Expression> expr = ParseEquality();

        while(token.type == TokenType::LogicalAnd)
        {
            const Token& oper = Consume();
            sptr<Expression> right = ParseEquality();
            expr = spnew<BinaryExpression>(oper.loc, scopes.GetCurrent(), oper.type, expr, right);
        }

        return expr;
    }

    sptr<Expression> ParseEquality()
    {
        sptr<Expression> expr = ParseComparison();

        while(IsAnyOf(token.type,
            TokenType::Equal,
            TokenType::NotEqual))
        {
            const Token& oper = Consume();
            sptr<Expression> right = ParseComparison();
            expr = spnew<BinaryExpression>(oper.loc, scopes.GetCurrent(), oper.type, expr, right);
        }

        return expr;
    }

    sptr<Expression> ParseComparison()
    {
        sptr<Expression> expr = ParseBitwiseOrExpression();
        
        while(
            token.IsType(TokenType::Less) ||
            token.IsType(TokenType::LessEqual) ||
            IsGreater() ||
            IsGreaterEqual())
        {
            SourceLocation loc;
            TokenType type;

            if(IsGreaterEqual())
            {
                loc = token.loc;
                type = TokenType::GreaterEqual;
                Consume(TokenType::Greater);
                Consume(TokenType::Assign);
            }
            else
            {
                loc = token.loc;
                type = token.type;
                Consume();
            }

            sptr<Expression> right = ParseBitwiseOrExpression();
            expr = spnew<BinaryExpression>(loc, scopes.GetCurrent(), type, expr, right);
        }

        return expr;
    }

    sptr<Expression> ParseBitwiseOrExpression()
    {
        sptr<Expression> expr = ParseBitwiseXorExpression();

        while(token.type == TokenType::BitOr)
        {
            const Token& oper = Consume();
            sptr<Expression> right = ParseBitwiseXorExpression();
            expr = spnew<BinaryExpression>(oper.loc, scopes.GetCurrent(), oper.type, expr, right);
        }

        return expr;
    }

    sptr<Expression> ParseBitwiseXorExpression()
    {
        sptr<Expression> expr = ParseBitwiseAndExpression();

        while(token.type == TokenType::BitXor)
        {
            const Token& oper = Consume();
            sptr<Expression> right = ParseBitwiseAndExpression();
            expr = spnew<BinaryExpression>(oper.loc, scopes.GetCurrent(), oper.type, expr, right);
        }

        return expr;
    }

    sptr<Expression> ParseBitwiseAndExpression()
    {
        sptr<Expression> expr = ParseShiftExpression();

        while(token.type == TokenType::BitAnd || token.type == TokenType::BitTest)
        {
            const Token& oper = Consume();
            sptr<Expression> right = ParseShiftExpression();
            expr = spnew<BinaryExpression>(oper.loc, scopes.GetCurrent(), oper.type, expr, right);
        }

        return expr;
    }

    sptr<Expression> ParseShiftExpression()
    {
        sptr<Expression> expr = ParseAddSub();
        
        while(token.IsType(TokenType::LeftShift) || IsRightShift())
        {
            SourceLocation loc;
            TokenType type;
            
            if(IsRightShift())
            {
                loc = token.loc;
                type = TokenType::RightShift;
                Consume(TokenType::Greater);
                Consume(TokenType::Greater);
            }
            else
            {
                loc = token.loc;
                type = token.type;
                Consume();
            }

            sptr<Expression> right = ParseAddSub();
            expr = spnew<BinaryExpression>(loc, scopes.GetCurrent(), type, expr, right);
        }

        return expr;
    }

    sptr<Expression> ParseAddSub()
    {
        sptr<Expression> expr = ParseMulDivMod();

        while(IsAnyOf(token.type, TokenType::Add, TokenType::Sub))
        {
            const Token& oper = Consume();
            sptr<Expression> right = ParseMulDivMod();
            expr = spnew<BinaryExpression>(oper.loc, scopes.GetCurrent(), oper.type, expr, right);
        }

        return expr;
    }

    sptr<Expression> ParseMulDivMod()
    {
        sptr<Expression> expr = ParseAwaitExpr();

        while(IsAnyOf(token.type, TokenType::Mul, TokenType::Div, TokenType::Mod))
        {
            const Token& oper = Consume();
            sptr<Expression> right = ParseAwaitExpr();
            expr = spnew<BinaryExpression>(oper.loc, scopes.GetCurrent(), oper.type, expr, right);
        }

        return expr;
    }

    sptr<Expression> ParseAwaitExpr()
    {
        if(token.IsKeyword(Keyword::Await))
        {
            const Token& oper = Consume();
            sptr<Expression> right = ParseAwaitExpr();
            return spnew<AwaitExpression>(oper.loc, scopes.GetCurrent(), right);
        }

        return ParsePrefixExpr();
    }

    sptr<Expression> ParsePrefixExpr()
    {
        if(IsAnyOf(token.type,
            TokenType::Add,
            TokenType::Sub,
            TokenType::Increment,
            TokenType::Decrement,
            TokenType::BitNot,
            TokenType::LogicalNot))
        {
            const Token& oper = Consume();
            sptr<Expression> right = ParsePrefixExpr();
            return spnew<PrefixExpression>(oper.loc, scopes.GetCurrent(), oper.type, right);
        }

        sptr<Expression> expr;
        expr = ParsePrimaryExpr();
        expr = ParsePostExpr(expr);
        return expr;
    }

    sptr<Expression> ParsePrimaryExpr()
    {
        sptr<Expression> expr;

        if (token.IsKeyword(Keyword::True))
        {
            expr = spnew<BooleanLiteralExpression>(token.loc, scopes.GetCurrent(), true);
            Consume(Keyword::True);
        }
        else if (token.IsKeyword(Keyword::False))
        {
            expr = spnew<BooleanLiteralExpression>(token.loc, scopes.GetCurrent(), false);
            Consume(Keyword::False);
        }
        else if (token.type == TokenType::IntegerLiteral)
        {
            expr = ParseIntegerLiteral();
        }
        else if (token.type == TokenType::NumberLiteral)
        {
            expr = ParseNumberLiteral();
        }
        else if (token.type == TokenType::StringLiteral)
        {
            expr = ParseStringLiteral();
        }
        else if (token.IsKeyword(Keyword::Null))
        {
            expr = spnew<NullLiteralExpression>(token.loc, scopes.GetCurrent());
            Consume(Keyword::Null);
        }
        else if (token.IsKeyword(Keyword::Functor))
        {
            expr = ParseFunctorExpr();
        }
        else if(token.IsKeyword(Keyword::New))
        {
            expr = ParseNewExpression();
        }
        else if(token.IsKeyword(Keyword::SizeOf))
        {
            expr = ParseSizeOfExpression();
        }
        // must be after keywords because keywords are identifiers
        else if (token.type == TokenType::Identifier)
        {
            expr = ParseIdentifierExpr();
        }
        else if (token.type == TokenType::LeftParen)
        {
            expr = ParseGroupExpr();
        }
        else
        {
            ENFORCE(false, token.loc, "expected expression");
        }

        return expr;
    }

    sptr<Expression> ParsePostExpr(sptr<Expression> expr)
    {
        while(true)
        {
            if (token.type == TokenType::Dot)
                expr = ParseMemberExpr(expr);
            else if (token.type == TokenType::LeftParen)
                expr = ParseCallExpr(expr);
            else if(token.type == TokenType::LeftBracket)
                expr = ParseIndexExpr(expr);
            else if (token.type == TokenType::Increment || token.type == TokenType::Decrement)
                expr = ParsePostfixExpr(expr);
            else if (token.IsKeyword(Keyword::As))
                expr = ParseAsExpr(expr);
            else if (token.IsKeyword(Keyword::Is))
                expr = ParseIsExpr(expr);
            else
                break;
        }

        return expr;
    }

    sptr<Expression> ParseMemberExpr(const sptr<Expression>& context)
    {
        context->isContext = true;
        Consume(TokenType::Dot);
        const Token& memberTok = Consume(TokenType::Identifier);
        return spnew<IdentifierExpression>(
            memberTok.loc, scopes.GetCurrent(), context, memberTok.GetIdentifier());
    }

    sptr<CallExpression> ParseCallExpr(const sptr<Expression>& target)
    {
        auto expr = spnew<CallExpression>(token.loc, scopes.GetCurrent(), target);

        Consume(TokenType::LeftParen);

        while (token.type != TokenType::RightParen)
        {
            expr->arguments.push_back( ParseExpression() );

            if (!TryConsume(TokenType::Comma))
                break;
        }

        Consume(TokenType::RightParen);

        return expr;
    }

    sptr<Expression> ParseIndexExpr(const sptr<Expression>& target)
    {
        target->isContext = true;
        auto expr = spnew<IndexExpression>(token.loc, scopes.GetCurrent(), target);

        Consume(TokenType::LeftBracket);
        expr->arg = ParseExpression();
        Consume(TokenType::RightBracket);

        return expr;
    }

    sptr<Expression> ParsePostfixExpr(const sptr<Expression>& left)
    {
        const Token& operTok = Consume();
        return spnew<PostfixExpression>(operTok.loc, scopes.GetCurrent(), left, operTok.type);
    }

    sptr<Expression> ParseAsExpr(const sptr<Expression>& left)
    {
        Consume(Keyword::As);
        sptr<TypeSpecifier> typeSpec = ParseTypeSpecifier();
        return spnew<AsExpression>(left->loc, scopes.GetCurrent(), left, typeSpec);
    }

    sptr<Expression> ParseIsExpr(const sptr<Expression>& left)
    {
        Consume(Keyword::Is);
        sptr<TypeSpecifier> typeSpec = ParseTypeSpecifier();
        return spnew<IsExpression>(left->loc, scopes.GetCurrent(), left, typeSpec);
    }

    sptr<Expression> ParseGroupExpr()
    {
        const Token& tok = Consume(TokenType::LeftParen);
        auto expr = ParseExpression();
        expr->loc = tok.loc;
        Consume(TokenType::RightParen);
        return expr;
    }

    sptr<IdentifierExpression> ParseIdentifierExpr()
    {
        const Token& identTok = Consume(TokenType::Identifier);

        bool isCoroutine = false;

        if(identTok.IsKeyword(Keyword::This))
        {
            auto owningFunc = scopes.GetCurrent()->owner->ToFunctionDefinition();
            ENFORCE(owningFunc && !owningFunc->isStatic &&
                ( owningFunc->parent->ToClassDefinition() || owningFunc->parent->ToStructDefinition() ),
                identTok.loc, "'this' can only be used in a non-static member function");

            isCoroutine = owningFunc->isCoroutine;
        }

        shared_string name = isCoroutine ? shared_string("$this") : identTok.GetIdentifier();
        return spnew<IdentifierExpression>(identTok.loc, scopes.GetCurrent(), name);
    }

    sptr<IntegerLiteralExpression> ParseIntegerLiteral()
    {
        auto num = spnew<IntegerLiteralExpression>(token.loc, scopes.GetCurrent());
        num->value = Consume(TokenType::IntegerLiteral).GetInteger();
        return num;
    }

    sptr<NumberLiteralExpression> ParseNumberLiteral()
    {
        auto num = spnew<NumberLiteralExpression>(token.loc, scopes.GetCurrent());
        num->value = Consume(TokenType::NumberLiteral).GetNumber();
        return num;
    }

    sptr<StringLiteralExpression> ParseStringLiteral()
    {
        auto expr = spnew<StringLiteralExpression>(token.loc, scopes.GetCurrent());
        expr->value = Consume(TokenType::StringLiteral).GetString();
        return expr;
    }

    sptr<NewExpression> ParseNewExpression()
    {
        auto expr = spnew<NewExpression>(token.loc, scopes.GetCurrent());
        
        Consume(Keyword::New);

        expr->typeSpec = ParseTypeSpecifier();

        if(TryConsume(TokenType::LeftBracket))
        {
            expr->typeSpec->arrayDimensions++;
            expr->argumentExpression = ParseExpression();
            Consume(TokenType::RightBracket);
        }

        if(expr->typeSpec->arrayDimensions != 0)
        {
            // parse array initializers
            if(TryConsume(TokenType::LeftBrace))
            {
                while(!token.IsType(TokenType::RightBrace))
                {
                    expr->arguments.push_back( ParseExpression() );

                    if (!TryConsume(TokenType::Comma))
                        break;
                }

                Consume(TokenType::RightBrace);
            }
        }
        else
        {
            Consume(TokenType::LeftBrace);

            while(!token.IsType(TokenType::RightBrace))
            {
                expr->arguments.push_back( ParseExpression() );

                if (!TryConsume(TokenType::Comma))
                    break;
            }

            Consume(TokenType::RightBrace);
        }

        return expr;
    }

    sptr<Expression> ParseSizeOfExpression()
    {
        auto loc = token.loc;

        Consume(Keyword::SizeOf);
        Consume(TokenType::LeftParen);
        auto typeSpec = ParseTypeSpecifier();
        Consume(TokenType::RightParen);

        auto expr = spnew<SizeOfExpression>(loc, scopes.GetCurrent(), typeSpec);
        return expr;
    }

    auto GetCaptures(sptr<FunctionDefinition>& func, Scope* searchStart)
    {
        // all of these are in the function body
        std::unordered_set<Definition*> definitions;

        // definitions for these may or may not be in the function body
        std::vector<sptr<IdentifierExpression>> identifiers;

        class CaptureScanner : public ASTVisitor
        {
        public:
            FunctionDefinition* func{};

            // all of these are in the function body
            std::unordered_set<Definition*> definitions;

            // definitions for these may or may not be in the function body
            std::vector<sptr<IdentifierExpression>> identifiers;

            virtual void Visit(const sptr<FunctionDefinition>& node) override
            {
                assert(node);
                func = node.get();
                
                for(const auto& param : node->GetChildren<ParameterDefinition>())
                    definitions.insert(param.get());

                //for (auto& def : node->scope->definitions)
                //    VisitChild(def);

                VisitChild(node->body);

                func = nullptr;
            }

            virtual void Visit(const sptr<ParameterDefinition>& node) override
            {
                ASTVisitor::Visit(node);
                definitions.insert(node.get());
            }

            virtual void Visit(const sptr<VariableDefinition>& node) override
            {
                ASTVisitor::Visit(node);
                definitions.insert(node.get());
            }

            virtual void Visit(const sptr<IdentifierExpression>& node) override
            {
                ASTVisitor::Visit(node);
                identifiers.push_back(node);
            }
        };

        // collect all definitions and identifiers in the function body
        CaptureScanner scanner;
        scanner.VisitChild(func);

        std::vector<sptr<IdentifierExpression>> captureIdentifiers;

        // figure out which identifiers are in the function body
        for(auto& ident : scanner.identifiers)
        {
            auto it = std::ranges::find_if(
                scanner.definitions, [&](const Definition* def){
                    return def->name == ident->value;
                });

            if(it == scanner.definitions.end())
                captureIdentifiers.push_back(ident);
        }

        std::vector<std::pair<sptr<IdentifierExpression>, sptr<Definition>>> captures;

        // search for the definition
        for(auto& ident : captureIdentifiers)
        {
            sptr<Definition> def = nullptr;
            
            // enclosing scope of the functor expression
            Scope* parent = searchStart;

            while(parent
                && !parent->owner->ToClassDefinition()
                && !parent->owner->ToSectionDefinition())
            {
                if(def = parent->FindDefinition(ident->value))
                    break;

                parent = parent->parent;
            }

            if(def)
            {
                captures.push_back( std::make_pair( ident, def ) );
            }
        }

        return captures;
    }

    sptr<Expression> ParseFunctorExpr()
    {
        // create functor class
        const Token& functorTok = Consume(Keyword::Functor);
        
        int id = nextUniqueId++;
        auto functorName = std::format("$functorClass{}", id);
        auto loc = functorTok.loc;

        Scope* currentScope = scopes.GetCurrent();

        ScopeStack temp;
        temp.Push(astRoot->global->scope.get());

        // swap in global scope stack
        std::swap(scopes, temp);

        auto functorClass = spnew<ClassDefinition>(loc, scopes.GetCurrent(), shared_string(functorName));
        scopes.GetCurrent()->AddDefinition(functorClass);
        
        scopes.Push(functorClass->scope.get()); // functor class

        auto func = ParseFunctionDefinition("invoke");
        functorClass->isFunctor = true;
        
        ENFORCE(!func->isExternal && !func->isStatic, func->loc, "a functor expression cannot have a storage class");
        ENFORCE(!!func->body, func->loc, "a functor expression must have a function body");

        scopes.Pop(); // functor class
        
        // restore scope stack
        std::swap(scopes, temp);

        auto newExpr = spnew<NewExpression>(loc, scopes.GetCurrent());
        newExpr->typeSpec = spnew<TypeSpecifier>(loc, scopes.GetCurrent(), shared_string(functorName));

        // add any captured variables to the functor
        auto captures = GetCaptures(func, currentScope);
        if(!captures.empty())
        {
            // add members to hold the captured variables
            // assign captured variables to members
            for(auto& capture : captures)
            {
                if(auto capturedVar = capture.second->ToVariableDefinition())
                {
                    auto enclosureScope = functorClass->scope.get();

                    // add a member to functor for captured variable
                    auto typeSpec = spnew<TypeSpecifier>(loc, enclosureScope, capturedVar->typeSpec->baseTypeName, capturedVar->typeSpec->arrayDimensions);
                    auto member = spnew<VariableDefinition>(loc, enclosureScope, typeSpec, capturedVar->name);
                    enclosureScope->AddDefinition(member);

                    auto arg = spnew<IdentifierExpression>(loc, scopes.GetCurrent(), capturedVar->name);
                    newExpr->arguments.push_back(arg);
                }
                else
                {
                    // not yet supported
                    assert(0);
                }
            }
        }
        
        return newExpr;
    }
};

} // fraze
