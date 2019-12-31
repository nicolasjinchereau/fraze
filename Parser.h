/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
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

const std::unordered_set<std::string> keywords = {
    "module",
    "return"
};

class Parser
{
    std::vector<Token> tokens;
    size_t index = 0;
    Token token;
public:

    Parser(const std::string& filename)
    {
        Lexer lexer(filename);
        lexer.Tokenize(tokens);
        token = tokens[0];
    }

    void Consume(TokenType tokenType, bool throwOnEOF)
    {
        assert(index < tokens.size() - 1);

        if(tokenType != TokenType::Invalid && token.type != tokenType)
            throw std::runtime_error("expected "s + Lexer::GetTokenName(tokenType));

        token = tokens[++index];
        
        if(throwOnEOF && token.type == TokenType::EndOfFile)
            throw std::runtime_error("unexpected end of file");
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

        Enforce(token.type == TokenType::Identifier && token.storage.stringValue == "module", "expected 'module'");
        Consume(true);
        
        Expect(TokenType::Identifier, "module name");
        ret->id = token.storage.stringValue;
        Consume(true);
        
        Consume(TokenType::LeftCurly, true);

        ParseModuleBody(ret);

        Consume(TokenType::RightCurly, false);

        return ret;
    }

    void ParseModuleBody(sptr<ModuleDefinition>& mod)
    {
        while (token.type != TokenType::RightCurly && token.type != TokenType::EndOfFile)
        {
            switch (token.type)
            {
            case TokenType::Identifier:
                // module SomeModule { .. }
                if (token.storage.stringValue == "module")
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
                        else
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
        varDecl->typeName = token.storage.stringValue;

        // consume type name
        Consume(true);

        Enforce(token.type == TokenType::Identifier, "expected variable name");
        varDecl->id = token.storage.stringValue;

        // consume variable name
        Consume(true);
        
        sptr<Expression> initializer;

        if (token.type == TokenType::Equals)
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
        func->returnTypeName = token.storage.stringValue;

        Consume(true);
        
        Expect(TokenType::Identifier, "a function name");
        func->name = token.storage.stringValue;
        Consume(true);
        
        Consume(TokenType::LeftParen, true);

        while (token.type != TokenType::RightParen && token.type != TokenType::EndOfFile)
        {
            // parse function parameter
            auto param = spnew<FunctionParameter>();

            Expect(TokenType::Identifier, "a type name");
            param->typeName = token.storage.stringValue;
            Consume(true);
            
            Expect(TokenType::Identifier, "a variable name");
            param->id = token.storage.stringValue;
            func->params.push_back(param);
            Consume(true);

            if(token.type == TokenType::Comma) {
                Consume(true);
            }
        }

        Consume(TokenType::RightParen, true);

        // following function definition, there should be a block statement
        Expect(TokenType::LeftCurly);

        func->body = ParseStatement();

        return func;
    }

    sptr<Statement> ParseStatement()
    {
        if (token.type == TokenType::LeftCurly)
        {
            // parse block statement
            Consume(true);

            auto block = spnew<BlockStatement>();

            while (token.type != TokenType::RightCurly && token.type != TokenType::EndOfFile)
            {
                auto stmt = ParseStatement();
                block->statements.push_back(std::move(stmt));
            }

            Consume(TokenType::RightCurly, false);

            return block;
        }
        
        if (token.type == TokenType::Identifier)
        {
            auto& id = token.storage.stringValue;
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
            token == TokenType::Plus ||
            token == TokenType::Minus ||
            token == TokenType::Multiply ||
            token == TokenType::Divide;
    }

    int GetPrecendence(TokenType tok)
    {
        switch (tok)
        {
        case TokenType::Plus:
        case TokenType::Minus:
            return 0;

        case TokenType::Multiply:
        case TokenType::Divide:
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
                func->name = token.storage.stringValue;
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
                var->name = token.storage.stringValue;
                Consume(true);

                return var;
            }
        }
        else if (token.type == TokenType::Integer)
        {
            // primary expression
            auto num = spnew<IntegerExpression>((int)token.storage.intValue);
            Consume(true);
            return num;
        }
        
        Enforce(false, "expected primary expression");
        return nullptr;
    }
};
