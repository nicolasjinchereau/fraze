/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2020 Nicolas Jinchereau. All rights reserved.
*  Licensed under the MIT License. See License.txt in the project root for license information.
*--------------------------------------------------------------------------------------------*/

#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>
#include <cfloat>
#include <memory>
#include <fstream>
#include <cassert>
#include <vector>
#include <variant>
#include <stdexcept>
#include <iostream>
#include <utf8.h>
using namespace std::string_literals;

enum class TokenType
{
    Invalid = -1, // invalid 
    EndOfFile,    // EOF
    LeftCurly,    // {
    RightCurly,   // }
    LeftBracket,  // [
    RightBracket, // ]
    LeftParen,    // (
    RightParen,   // )
    Equals,       // =
    Plus,         // +
    Minus,        // -
    Multiply,     // *
    Divide,       // /
    Dot,          // .
    Comma,        // ,
    Colon,        // :
    Semicolon,    // ;
    String,       // "abcd1234"
    Integer,      // 123
    Float,        // 12.34
    Boolean,      // true false
    Null,         // null
    Identifier,   // _asdf3423
};

struct Token
{
    union ValueType {
        int64_t intValue;
        double floatValue;
        bool boolValue;
        char charValue;
        std::string stringValue;

        ValueType(){}
        ~ValueType(){}
    };

    TokenType type = TokenType::EndOfFile;
    size_t pos = -1;
    ValueType storage;

    Token() = default;

    Token(const Token& tok) : type(tok.type), pos(tok.pos)
    {
        if (type == TokenType::String)
            new (&storage.stringValue) std::string(tok.storage.stringValue);
        else
            memcpy(&storage, &tok.storage, sizeof(ValueType));
    }

    Token(Token&& tok) noexcept : type(tok.type), pos(tok.pos)
    {
        if (type == TokenType::String)
            new (&storage.stringValue) std::string(std::move(tok.storage.stringValue));
        else
            memcpy(&storage, &tok.storage, sizeof(ValueType));
    }

    Token& operator=(const Token& tok)
    {
        if (type == TokenType::String) {
            typedef std::string stype;
            storage.stringValue.~stype();
        }

        type = tok.type;
        pos = tok.pos;

        if (type == TokenType::String)
            new (&storage.stringValue) std::string(tok.storage.stringValue);
        else
            memcpy(&storage, &tok.storage, sizeof(ValueType));

        return *this;
    }

    Token& operator=(Token&& tok) noexcept
    {
        if (type == TokenType::String) {
            typedef std::string stype;
            storage.stringValue.~stype();
        }

        type = tok.type;
        pos = tok.pos;

        if (type == TokenType::String)
            new (&storage.stringValue) std::string(std::move(tok.storage.stringValue));
        else
            memcpy(&storage, &tok.storage, sizeof(ValueType));

        return *this;
    }

    Token(TokenType type, size_t pos, const std::string& value)
        : type(type), pos(pos)
    {
        new (&storage.stringValue) std::string(value);
    }

    Token(TokenType type, size_t pos, std::string&& value)
        : type(type), pos(pos)
    {
        new (&storage.stringValue) std::string(std::move(value));
    }

    Token(TokenType type, size_t pos, int64_t value)
        : type(type), pos(pos)
    {
        storage.intValue = value;
    }

    Token(TokenType type, size_t pos, double value)
        : type(type), pos(pos)
    {
        storage.floatValue = value;
    }

    Token(TokenType type, size_t pos, bool value)
        : type(type), pos(pos)
    {
        storage.boolValue = value;
    }

    Token(TokenType type, size_t pos, char value)
        : type(type), pos(pos)
    {
        storage.charValue = value;
    }

    Token(TokenType type, size_t pos, std::nullptr_t value)
        : type(type), pos(pos)
    {
        storage.intValue = 0;
    }

    ~Token()
    {
        if (type == TokenType::String) {
            typedef std::string stype;
            storage.stringValue.~stype();
        }
    }
};

class Lexer
{
    size_t line;
    size_t column;
    size_t offset;
    char32_t value;
    std::basic_string<int, std::char_traits<int>> chars;
    int tabLength = 4;

public:

    static const std::string& GetTokenName(TokenType type)
    {
        static const std::unordered_map<int, std::string> tokenNames {
            { (int)TokenType::Invalid, "invalid token" },
            { (int)TokenType::EndOfFile, "end of file" },
            { (int)TokenType::LeftCurly, "{" },
            { (int)TokenType::RightCurly, "}" },
            { (int)TokenType::LeftBracket, "[" },
            { (int)TokenType::RightBracket, "]" },
            { (int)TokenType::LeftParen, "(" },
            { (int)TokenType::RightParen, ")" },
            { (int)TokenType::Equals, "=" },
            { (int)TokenType::Plus, "+" },
            { (int)TokenType::Minus, "-" },
            { (int)TokenType::Multiply, "*" },
            { (int)TokenType::Divide, "/" },
            { (int)TokenType::Dot, "." },
            { (int)TokenType::Comma, "," },
            { (int)TokenType::Colon, ":" },
            { (int)TokenType::Semicolon, ";" },
            { (int)TokenType::String, "string" },
            { (int)TokenType::Integer, "integer" },
            { (int)TokenType::Float, "float" },
            { (int)TokenType::Boolean, "boolean" },
            { (int)TokenType::Null, "null" },
            { (int)TokenType::Identifier, "identifier" }
        };
        
        return tokenNames.at((int)type);
    }

    bool endOfFile() const {
        return offset == chars.size();
    }

    char32_t getValue() const {
        return value;
    }

    size_t getOffset() const {
        return offset;
    }

    size_t contentLength() const {
        return chars.size();
    }

    Lexer(const std::string& filename)
    {
        std::ifstream fin(filename, std::ios::in | std::ios::binary);

        if (!fin.good())
            throw std::runtime_error("failed to open file: "s + filename);

        fin.seekg(0, std::ios::end);
        auto sz = (size_t)fin.tellg();
        fin.seekg(0, std::ios::beg);

        if (sz == 0)
            throw std::runtime_error("file is empty: "s + filename);

        auto buffer = std::unique_ptr<char[]>(new char[sz]);
        fin.read(buffer.get(), sz);

        utf8::utf8to32(buffer.get(), buffer.get() + sz, std::back_inserter(chars));

        line = 0;
        column = 0;
        offset = 0;
        value = chars[0];
    }

    static std::vector<Token> Tokenize(const std::string& filename)
    {
        std::vector<Token> tokens;
        Lexer lexer(filename);

        do {
            tokens.push_back(lexer.GetNextToken());
        } while (!lexer.endOfFile());

        return tokens;
    }

    void Tokenize(std::vector<Token>& outTokens)
    {
        assert(offset == 0);

        do {
            outTokens.push_back(GetNextToken());
        } while (!endOfFile());
    }

    std::vector<Token> Tokenize()
    {
        std::vector<Token> tokens;
        Tokenize(tokens);
        return tokens;
    }

    Token GetNextToken()
    {
        SkipWhitespace();

        auto pos = offset;

        if (offset == chars.size())
            return Token(TokenType::EndOfFile, pos, (char)EOF);
        
        switch (value)
        {
        case '{':
            SkipChar();
            return Token(TokenType::LeftCurly, pos, '{');
        case '}':
            SkipChar();
            return Token(TokenType::RightCurly, pos, '}');
        case '[':
            SkipChar();
            return Token(TokenType::LeftBracket, pos, '[');
        case ']':
            SkipChar();
            return Token(TokenType::RightBracket, pos, ']');
        case '(':
            SkipChar();
            return Token(TokenType::LeftParen, pos, '(');
        case ')':
            SkipChar();
            return Token(TokenType::RightParen, pos, ')');
        case '=':
            SkipChar();
            return Token(TokenType::Equals, pos, '=');
        case '+':
            SkipChar();
            return Token(TokenType::Plus, pos, '+');
        case '-':
            SkipChar();
            return Token(TokenType::Minus, pos, '-');
        case '*':
            SkipChar();
            return Token(TokenType::Multiply, pos, '*');
        case '/':
            SkipChar();
            return Token(TokenType::Divide, pos, '/');
        case ',':
            SkipChar();
            return Token(TokenType::Comma, pos, ',');
        case ':':
            SkipChar();
            return Token(TokenType::Colon, pos, ':');
        case ';':
            SkipChar();
            return Token(TokenType::Semicolon, pos, ',');
        case '\"':
            return GetStringToken();
        case '.':
            if(!isdigit(chars[offset + 1])) {
                SkipChar();
                return Token(TokenType::Dot, pos, ',');
            }
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return GetNumberToken();
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
        case '_':
            return GetIdentifierToken();
        default:
            throw std::runtime_error("found unexpected input: "s + (char)value);
        }
    }

private:
    
    void SkipWhitespace()
    {
        while (offset != chars.size())
        {
            if (value == ' ') // spaces
            {
                ++column;
            }
            else if (value == '\t')
            {
                column += tabLength;
            }
            else if (value == '\n') // new line
            {
                ++line;
                column = 0;
            }
            else if (value == '\v' || value == '\f' || value == '\r')
            {
                // ignore
            }
            else
            {
                // found non-whitespace character
                break;
            }

            SkipChar();
        }
    }

    char32_t PeekNextChar() {
        assert(offset < chars.size());
        return chars[offset + 1];
    }

    void SkipChar() {
        assert(offset <= chars.size());
        value = chars[++offset];
    }

    void SkipChars(int count) {
        assert(offset <= chars.size() - count);
        value = chars[offset += count];
    }

    bool IsStartOfNumber(char32_t c) {
        return (c >= '0' && c <= '9') || c == '-' || c == '+';
    }

    bool IsStartOf(char32_t c) {
        return (c >= '0' && c <= '9') || c == '-' || c == '+';
    }

    Token GetStringToken()
    {
        assert(value == '\"');
        
        auto start = offset;
        SkipChar();

        std::u32string str;
        
        while (offset != chars.size())
        {
            if (value == '\"')
            {
                SkipChar();

                std::string utf8str;
                utf8str.reserve(str.size());
                utf8::utf32to8(str.begin(), str.end(), std::back_inserter(utf8str));
                return Token(TokenType::String, start, utf8str);
            }
            else if (value == '\\')
            {
                SkipChar();

                if (offset == chars.size())
                    throw std::runtime_error("unexpected end of input");

                if (value == '\"') {
                    SkipChar();
                    str += '\"';
                }
                else if (value == '\\') {
                    SkipChar();
                    str += '\\';
                }
                else if (value == '/') {
                    SkipChar();
                    str += '/';
                }
                else if (value == 'b') {
                    SkipChar();
                    str += '\b';
                }
                else if (value == 'f') {
                    SkipChar();
                    str += '\f';
                }
                else if (value == 'n') {
                    SkipChar();
                    str += '\n';
                }
                else if (value == 'r') {
                    SkipChar();
                    str += '\r';
                }
                else if (value == 't') {
                    SkipChar();
                    str += '\t';
                }
                else if (value == 'u')
                {
                    SkipChar();

                    if (offset > chars.size() - 4)
                        throw std::runtime_error("unexpected end of input");

                    char hex[4];

                    for (int i = 0; i < 4; ++i)
                    {
                        if (!isxdigit(value))
                            throw std::runtime_error("invalid unicode escape sequence");

                        hex[i] = (char)value;
                        SkipChar();
                    }
                    
                    str += (char32_t)std::strtoul(hex, nullptr, 16);
                }
                else
                {
                    str += value;
                    SkipChar();
                }
            }
            else
            {
                str += value;
                SkipChar();
            }
        }

        assert(offset == chars.size());
        throw std::runtime_error("unexpected end of input");
    }

    Token GetNumberToken()
    {
        size_t numberStart = offset;
        
        //int sign = 1;

        //if (value == '-') {
        //    sign = -1;
        //    SkipChar();
        //}
        //else if (value == '+') {
        //    SkipChar();
        //}

        // MANTISSA
        size_t mantStart = offset;
        int64_t mantissa = 0;
        int64_t exponent = 0;
        bool hasDecimal = false;
        
        while (offset < chars.size())
        {
            if (isdigit(value)) {

                if (hasDecimal)
                    --exponent;

                mantissa = mantissa * 10 + (value - U'0');
                SkipChar();
            }
            else if (value == '.' && !hasDecimal) {
                hasDecimal = true;
                SkipChar();
                continue;
            }
            else {
                break;
            }
        }

        //if (offset - mantStart == 0)
        //    throw runtime_error("invalid number (no whole part)");

        if (value == 'e' || value == 'E')
        {
            SkipChar();

            int expSign = 1;

            if (value == '-') {
                expSign = -1;
                SkipChar();
            }
            else if (value == '+') {
                SkipChar();
            }

            int64_t exp = 0;

            while (offset < chars.size() && isdigit(value)) {
                exp = exp * 10 + (value - U'0');
                SkipChar();
            }

            exponent += exp * expSign;

            // exponent is required if 'e' or 'E' are present
            //if (expStart == expEnd)
            //    throw runtime_error("invalid number");
        }

        if (exponent >= 0) {
            //auto wholeNumber = mantissa * (int64_t)(std::pow(10, exponent) + 0.5) * sign;
            auto wholeNumber = mantissa * (int64_t)(std::pow(10, exponent) + 0.5);
            return Token(TokenType::Integer, numberStart, wholeNumber);
        }

        //auto fractNumber = (double)std::copysign(mantissa * std::pow(10.0L, exponent), sign);
        auto fractNumber = (double)(mantissa * std::pow(10.0L, exponent));
        return Token(TokenType::Float, numberStart, fractNumber);
    }

    Token GetIdentifierToken()
    {
        assert(isalpha(value) || value == '_');

        size_t start = offset;
        std::u32string str;

        do {
            str.push_back(value);
            SkipChar();
        } while (offset != chars.size() && (isalnum(value) || value == '_'));

        std::string utf8str;
        utf8str.reserve(str.size());
        utf8::utf32to8(str.begin(), str.end(), std::back_inserter(utf8str));

        return Token(TokenType::Identifier, start, utf8str);
    }
};
