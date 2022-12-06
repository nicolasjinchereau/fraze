/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2022 Nicolas Jinchereau. All rights reserved.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include "Lexer.h"
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <stdexcept>
#include <utility>
#include <cassert>
#include <unordered_map>
#include <filesystem>
#include <format>
#include "Exception.h"
#include "Pointers.h"
#include "TranslationUnit.h"
#include "FunctionDefinition.h"
#include "FunctionParameter.h"
#include "VariableDeclaration.h"
#include "ReturnStatement.h"
#include "Statement.h"
#include "BlockStatement.h"
#include "Expression.h"
#include "ExpressionStatement.h"
#include "DeclarationStatement.h"
#include "IntegerExpression.h"
#include "FunctionExpression.h"
#include "VariableExpression.h"
#include "BinaryExpression.h"

class Parser
{
    Lexer lexer;
    std::vector<Token> tokens;
    size_t index = 0;
    Token token;
public:

    Parser(const std::filesystem::path& path)
        : lexer(path)
    {
        lexer.Tokenize(tokens);
        token = tokens[0];
    }

    void Consume(TokenType tokenType, bool throwOnEOF)
    {
        assert(index < tokens.size() - 1);

        if(tokenType != TokenType::Invalid && token.type != tokenType)
            throw Exception("expected "s + Lexer::GetTokenName(tokenType));

        token = tokens[++index];
        
        if(throwOnEOF && token.type == TokenType::EndOfFile)
            throw Exception("unexpected end of file");
    }

    void Consume(bool throwOnEOF) {
        Consume(TokenType::Invalid, throwOnEOF);
    }

    void Expect(TokenType tokenType, const std::string& tokenNameSubstitute = std::string())
    {
        if(tokenType != TokenType::Invalid && token.type != tokenType)
            throw std::runtime_error("expected "s + (!tokenNameSubstitute.empty() ? tokenNameSubstitute : Lexer::GetTokenName(tokenType)));
    }

    Token& currentToken() {
        assert(index < tokens.size());
        return tokens[index];
    }

    Token& PeekToken(int ahead = 1) {
        assert(index < tokens.size() - ahead);
        return tokens[index + ahead];
    }

    void Enforce(bool condition, const std::string& error = std::string())
    {
        if (!condition)
            throw std::runtime_error(error);
    }

    sptr<TranslationUnit> ParseTranslationUnit()
    {
        auto ret = spnew<TranslationUnit>();
        
        ret->rootModule = spnew<ModuleDefinition>("global");
        ParseModuleBody(ret->rootModule);

        return ret;
    }

    sptr<ModuleDefinition> ParseModule()
    {
        auto ret = spnew<ModuleDefinition>();
        
        Enforce(token.IsKeyword(Keyword::Module), "expected 'module'");
        Consume(true);
        
        Expect(TokenType::Identifier, "module name");
        ret->id = token.GetString();
        Consume(true);
        
        Consume(TokenType::LeftBrace, true);

        ParseModuleBody(ret);

        Consume(TokenType::RightBrace, false);

        return ret;
    }

    void ParseModuleBody(sptr<ModuleDefinition>& mod)
    {
        while (token.type != TokenType::RightBrace && token.type != TokenType::EndOfFile)
        {
            switch (token.type)
            {
            case TokenType::Identifier:
                // module SomeModule { .. }
                if (token.IsKeyword(Keyword::Module))
                {
                    auto nestedMod = ParseModule();
                    mod->modules.push_back(nestedMod);
                    break;
                }
                else
                {
                    if (PeekToken(1).type == TokenType::Identifier)
                    {
                        auto next = PeekToken(2).type;

                        // int Fun(params)
                        if (next == TokenType::LeftParen)
                        {
                            auto func = ParseFunctionDefinition();
                            mod->functions.push_back(std::move(func));
                        }
                        // int Variable
                        else if (next == TokenType::Assign || next == TokenType::Semicolon)
                        {
                            auto var = ParseVariableDeclaration();
                            mod->variables.push_back(var);
                        }
                    }
                }
                break;

            default:
                Enforce(false, "expected function or variable name");
                break;
            }
        }
    }

    sptr<VariableDeclaration> ParseVariableDeclaration()
    {
        auto varDecl = spnew<VariableDeclaration>();
        
        Expect(TokenType::Identifier, "a type name");
        varDecl->typeName = token.GetString();

        // consume type name
        Consume(true);

        Enforce(token.type == TokenType::Identifier, "expected variable name");
        varDecl->id = token.GetString();

        // consume variable name
        Consume(true);
        
        sptr<Expression> initializer;

        if (token.type == TokenType::Assign)
        {
            // consume '=' operator
            Consume(true);

            // parse expression up to the next semicolon
            initializer = ParseExpression(0);
        }
        else
        {
            // default value
            initializer = spnew<Expression>();
        }

        // consume semicolon
        Consume(TokenType::Semicolon, false);

        varDecl->initializer = std::move(initializer);

        return varDecl;
    }

    sptr<FunctionDefinition> ParseFunctionDefinition()
    {
        auto func = spnew<FunctionDefinition>();

        Expect(TokenType::Identifier, "a type name");
        func->returnTypeName = token.GetString();

        Consume(true);
        
        Expect(TokenType::Identifier, "a function name");
        func->name = token.GetString();
        Consume(true);
        
        Consume(TokenType::LeftParen, true);

        while (token.type != TokenType::RightParen && token.type != TokenType::EndOfFile)
        {
            // parse function parameter
            auto param = spnew<FunctionParameter>();

            Expect(TokenType::Identifier, "a type name");
            param->typeName = token.GetString();
            Consume(true);
            
            Expect(TokenType::Identifier, "a variable name");
            param->id = token.GetString();
            func->params.push_back(param);
            Consume(true);

            if(token.type == TokenType::Comma) {
                Consume(true);
            }
        }

        Consume(TokenType::RightParen, true);

        // following function definition, there should be a block statement
        Expect(TokenType::LeftBrace);

        func->body = ParseStatement();

        return func;
    }

    sptr<Statement> ParseStatement()
    {
        if (token.type == TokenType::LeftBrace)
        {
            // parse block statement
            Consume(true);

            auto block = spnew<BlockStatement>();

            while (token.type != TokenType::RightBrace && token.type != TokenType::EndOfFile)
            {
                auto stmt = ParseStatement();
                block->statements.push_back(std::move(stmt));
            }

            Consume(TokenType::RightBrace, false);

            return block;
        }
        
        if (token.type == TokenType::Identifier)
        {
            auto& id = token.GetString();
            if (id == "return")
            {
                // consume "return" keyword
                Consume(true);
                
                auto stmt = spnew<ReturnStatement>();

                // parse return expression
                stmt->expression = ParseExpression(0);

                // final semicolon
                Consume(TokenType::Semicolon, false);

                return stmt;
            }
            else if (PeekToken(1).type == TokenType::Identifier)
            {
                auto stmt = spnew<DeclarationStatement>();
                stmt->variableDeclaration = ParseVariableDeclaration(); // probably shouldn't consume semicolon
                return stmt;
            }
        }

        // try to parse expression up to the next semicolon
        auto stmt = spnew<ExpressionStatement>();
        stmt->expression = ParseExpression(0);

        // final semicolon
        Consume(TokenType::Semicolon, false);

        return stmt;
    }

    bool IsBinaryOperator(TokenType token)
    {
        return
            token == TokenType::Add ||
            token == TokenType::Sub ||
            token == TokenType::Mul ||
            token == TokenType::Div;
    }

    int GetPrecendence(TokenType tok)
    {
        switch (tok)
        {
        case TokenType::Add:
        case TokenType::Sub:
            return 0;

        case TokenType::Mul:
        case TokenType::Div:
            return 1;

        default:
            return -1;
        }
    }

    sptr<Expression> ParseExpression(int precedence)
    {
        sptr<Expression> exp = ParseExpressionOperand();

        while (IsBinaryOperator(token.type) && GetPrecendence(token.type) >= precedence)
        {
            auto op = token.type;
            Consume(true);

            auto right = ParseExpression(GetPrecendence(op) + 1);
            exp = spnew<BinaryExpression>(op, exp, right);
        }

        return exp;
    }

    sptr<Expression> ParseExpressionOperand()
    {
        if (token.type == TokenType::LeftParen)
        {
            // sub-expression
            Consume(true);

            auto exp = ParseExpression(0);

            Consume(TokenType::RightParen, false);

            return exp;
        }
        else if (token.type == TokenType::Identifier)
        {
            if (PeekToken(1).type == TokenType::LeftParen)
            {
                // function
                auto func = spnew<FunctionExpression>();

                // function name
                func->name = token.GetString();
                Consume(true);

                // '('
                Consume(TokenType::LeftParen, true);

                while (token.type != TokenType::RightParen && token.type != TokenType::EndOfFile)
                {
                    auto arg = ParseExpression(0);
                    func->arguments.push_back(std::move(arg));

                    if (token.type == TokenType::Comma)
                        Consume(true);
                }

                // ')'
                Consume(TokenType::RightParen, true);

                return func;
            }
            else
            {
                // variable
                auto var = spnew<VariableExpression>();

                // variable name
                var->name = token.GetString();
                Consume(true);

                return var;
            }
        }
        else if (token.type == TokenType::IntegerLiteral)
        {
            // primary expression
            auto num = spnew<IntegerExpression>((int)token.GetInt());
            Consume(true);
            return num;
        }
        
        Enforce(false, "expected primary expression");
        return nullptr;
    }
};
