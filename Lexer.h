/*---------------------------------------------------------------------------------------------
*  Copyright (c) 2022 Nicolas Jinchereau. All rights reserved.
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
#include <variant>
#include <filesystem>
#include <charconv>

using namespace std::string_literals;

enum class TokenType : int8_t
{
    Invalid = -1,     //  invalid
    EndOfFile,        //  EOF

    LeftBrace,        //  {
    RightBrace,       //  }
    LeftBracket,      //  [
    RightBracket,     //  ]
    LeftParen,        //  (
    RightParen,       //  )

    Assign,           //  =

    Add,              //  +
    AddAssign,        //  +=
    AddOne,           //  ++
    Sub,              //  -
    SubAssign,        //  -=
    SubOne,           //  --
    Mul,              //  *
    MulAssign,        //  *=
    Div,              //  /
    DivAssign,        //  /=
    Mod,              //  %
    ModAssign,        //  %=

    LeftShift,        //  <<
    LeftShiftAssign,  //  <<=
    RightShift,       //  >>
    RightShiftAssign, //  >>=

    BitOr,            //  |
    BitOrAssign,      //  |=
    BitAnd,           //  &
    BitAndAssign,     //  &=
    BitNot,           //  ~
    BitNotAssign,     //  ~=
    BitXor,           //  ^
    BitXorAssign,     //  ^=

    LogicalAnd,       //  &&
    LogicalOr,        //  ||
    LogicalNot,       //  !
    QuestionMark,     //  ?

    Equal,            //  ==
    NotEqual,         //  !=
    Less,             //  <
    LessEqual,        //  <=
    Greater,          //  >
    GreaterEqual,     //  >=

    Dot,              //  .
    Comma,            //  ,
    Colon,            //  :
    Semicolon,        //  ;

    CharacterLiteral, //  'a'
    IntegerLiteral,   //  123
    FloatLiteral,     //  12.34
    BitLiteral,       //  0b1010
    HexLiteral,       //  0xFF99
    StringLiteral,    //  "abcd1234"

    Identifier        //  _asdf3423
};

enum class Keyword : uint8_t
{
    Module,
    Import,
    If,
    Else,
    Class,
    New,
    Void,
    True,
    False,
    Return
};

struct Token
{
    TokenType type = TokenType::EndOfFile;
    std::string_view chars;
    std::variant<nullptr_t, char32_t, int64_t, uint64_t, float, double, std::string, Keyword> value;

    Token() = default;

    Token(TokenType type, std::string_view chars)
        : type(type), chars(chars), value() { }

    Token(std::string_view chars, char32_t characterLiteral)
        : type(TokenType::CharacterLiteral), chars(chars), value(characterLiteral) { }

    Token(std::string_view chars, int64_t integerLiteral)
        : type(TokenType::IntegerLiteral), chars(chars), value(integerLiteral) { }
    
    Token(std::string_view chars, uint64_t integerLiteral)
        : type(TokenType::IntegerLiteral), chars(chars), value(integerLiteral) { }

    Token(std::string_view chars, float floatLiteral)
        : type(TokenType::FloatLiteral), chars(chars), value(floatLiteral) { }

    Token(std::string_view chars, double floatLiteral)
        : type(TokenType::FloatLiteral), chars(chars), value(floatLiteral) { }

    Token(std::string_view chars, const std::string& stringLiteral)
        : type(TokenType::StringLiteral), chars(chars), value(stringLiteral) { }

    Token( std::string_view chars, std::string&& stringLiteral)
        : type(TokenType::FloatLiteral), chars(chars), value(std::move(stringLiteral)) { }

    Token(std::string_view chars, Keyword keyword)
        : type(TokenType::Identifier), chars(chars), value(keyword) { }

    bool IsCharacter() const { return std::holds_alternative<char32_t>(value); }
    bool IsInt() const { return std::holds_alternative<int64_t>(value); }
    bool IsUInt() const { return std::holds_alternative<uint64_t>(value); }
    bool IsFloat() const { return std::holds_alternative<float>(value); }
    bool IsDouble() const { return std::holds_alternative<double>(value); }
    bool IsString() const { return std::holds_alternative<std::string>(value); }
    bool IsKeyword(Keyword keyword) const { return type == TokenType::Identifier && std::get<Keyword>(value) == keyword; }
    
    char32_t GetCharacter() const { return std::get<char32_t>(value); }
    int64_t GetInt() const { return std::get<int64_t>(value); }
    uint64_t GetUInt() const { return std::get<uint64_t>(value); }
    float GetFloat() const { return std::get<float>(value); }
    double GetDouble() const { return std::get<double>(value); }
    const std::string& GetString() const { return std::get<std::string>(value); }
    Keyword GetKeyword() const { return std::get<Keyword>(value); }

    std::string_view GetSource() const { return chars; }
};

const std::unordered_map<std::string, const Keyword> Keywords
{
    { "module", Keyword::Module },
    { "import", Keyword::Import },
    { "if",     Keyword::If },
    { "else",   Keyword::Else },
    { "class",  Keyword::Class },
    { "new",    Keyword::New },
    { "void",   Keyword::Void },
    { "true",   Keyword::True },
    { "false",  Keyword::False },
    { "return", Keyword::Return }
};

const std::unordered_map<Keyword, const std::string> KeywordNames
{
    { Keyword::Module, "module" },
    { Keyword::Import, "import" },
    { Keyword::If, "if" },
    { Keyword::Else, "else" },
    { Keyword::Class, "class" },
    { Keyword::New, "new" },
    { Keyword::Void, "void" },
    { Keyword::True, "true" },
    { Keyword::False, "false" },
    { Keyword::Return, "return" }
};

const std::unordered_map<TokenType, const std::string> TokenNames
{
    { TokenType::Invalid,          "invalid token" },
    { TokenType::EndOfFile,        "end of file" },

    { TokenType::LeftBrace,        "{" },
    { TokenType::RightBrace,       "}" },
    { TokenType::LeftBracket,      "[" },
    { TokenType::RightBracket,     "]" },
    { TokenType::LeftParen,        "(" },
    { TokenType::RightParen,       ")" },

    { TokenType::Assign,           "=" },

    { TokenType::Add,              "+" },
    { TokenType::AddAssign,        "+=" },
    { TokenType::AddOne,           "++" },
    { TokenType::Sub,              "-" },
    { TokenType::SubAssign,        "-=" },
    { TokenType::SubOne,           "--" },
    { TokenType::Mul,              "*" },
    { TokenType::MulAssign,        "*=" },
    { TokenType::Div,              "/" },
    { TokenType::DivAssign,        "/=" },

    { TokenType::LeftShift,        "<<" },
    { TokenType::LeftShiftAssign,  "<<=" },
    { TokenType::RightShift,       ">>" },
    { TokenType::RightShiftAssign, ">>=" },

    { TokenType::Mod,              "%" },
    { TokenType::ModAssign,        "%=" },

    { TokenType::BitOr,            "|" },
    { TokenType::BitOrAssign,      "|=" },
    { TokenType::BitAnd,           "&" },
    { TokenType::BitAndAssign,     "&=" },
    { TokenType::BitNot,           "~" },
    { TokenType::BitNotAssign,     "~=" },
    { TokenType::BitXor,           "^" },
    { TokenType::BitXorAssign,     "^=" },

    { TokenType::Equal,            "==" },
    { TokenType::NotEqual,         "!=" },
    { TokenType::Less,             "<" },
    { TokenType::LessEqual,        "<=" },
    { TokenType::Greater,          ">" },
    { TokenType::GreaterEqual,     ">=" },
    { TokenType::QuestionMark,     "?" },

    { TokenType::Dot,              "." },
    { TokenType::Comma,            "," },
    { TokenType::Colon,            ":" },
    { TokenType::Semicolon,        ";" },

    { TokenType::CharacterLiteral, "character literal" },
    { TokenType::IntegerLiteral,   "integer literal" },
    { TokenType::FloatLiteral,     "float literal" },
    { TokenType::BitLiteral,       "binary literal" },
    { TokenType::HexLiteral,       "hex literal" },
    { TokenType::StringLiteral,    "string literal" },

    { TokenType::Identifier,       "identifier" }
};

class Lexer
{
    std::string chars;
    std::string::iterator pos;
    std::string::iterator next;
    char32_t value{};
    size_t line = 0;
    size_t column = 0;

    constexpr static int TabLength = 4;
public:
    Lexer(const std::filesystem::path& path)
        : Lexer(ReadFile(path))
    {
    }

    Lexer(const std::string& text)
        : Lexer(std::string(text))
    {
    }

    Lexer(std::string&& text)
        : chars(std::move(text))
    {
        pos = chars.begin();
        next = pos;
        value = utf8::next(next, chars.end());
    }

    std::vector<Token> Tokenize()
    {
        std::vector<Token> tokens;
        Tokenize(tokens);
        return tokens;
    }

    void Tokenize(std::vector<Token>& outTokens)
    {
        outTokens.clear();

        do {
            outTokens.push_back(GetNextToken());
        } while (pos != chars.end());
    }
    
    static const std::string& GetTokenName(TokenType type) {
        auto it = TokenNames.find(type);
        return it != TokenNames.end() ? it->second : TokenNames.at(TokenType::Invalid);
    }

private:
    Token GetNextToken()
    {
        SkipWhitespace();

        if (pos == chars.end())
            return Token();
        
        auto start = pos;

        switch (value)
        {
        case '{':
            SkipChar();
            return Token(TokenType::LeftBrace, { start, pos });
        case '}':
            SkipChar();
            return Token(TokenType::RightBrace, { start, pos });
        case '[':
            SkipChar();
            return Token(TokenType::LeftBracket, { start, pos });
        case ']':
            SkipChar();
            return Token(TokenType::RightBracket, { start, pos });
        case '(':
            SkipChar();
            return Token(TokenType::LeftParen, { start, pos });
        case ')':
            SkipChar();
            return Token(TokenType::RightParen, { start, pos });
        case '=':
            SkipChar();

            if(value == '=') {
                SkipChar();
                return Token(TokenType::Equal, { start, pos });
            }
            else {
                return Token(TokenType::Assign, { start, pos });
            }

        case '+':
            SkipChar();

            if(value == '=') {
                SkipChar();
                return Token(TokenType::AddAssign, { start, pos });
            }
            else if(value == '+') {
                SkipChar();
                return Token(TokenType::AddOne, { start, pos });
            }
            else {
                return Token(TokenType::Add, { start, pos });
            }

        case '-':
            SkipChar();

            if(value == '=') {
                SkipChar();
                return Token(TokenType::SubAssign, { start, pos });
            }
            else if(value == '-') {
                SkipChar();
                return Token(TokenType::SubOne, { start, pos });
            }
            else {
                return Token(TokenType::Sub, { start, pos });
            }

        case '*':
            SkipChar();

            if(value == '=') {
                SkipChar();
                return Token(TokenType::MulAssign, { start, pos });
            }
            else {
                return Token(TokenType::Mul, { start, pos });
            }

        case '/':
            SkipChar();

            if(value == '=') {
                SkipChar();
                return Token(TokenType::DivAssign, { start, pos });
            }
            else {
                return Token(TokenType::Div, { start, pos });
            }

        case '%':
            SkipChar();

            if(value == '=') {
                SkipChar();
                return Token(TokenType::ModAssign, { start, pos });
            }
            else {
                return Token(TokenType::Mod, { start, pos });
            }

        case '<':
            SkipChar();

            if(value == '<') {
                SkipChar();

                if(value == '=') {
                    SkipChar();
                    return Token(TokenType::LeftShiftAssign, { start, pos });
                }
                else {
                    return Token(TokenType::LeftShift, { start, pos });
                }
            }
            else {
                if(value == '=') {
                    SkipChar();
                    return Token(TokenType::LessEqual, { start, pos });
                }
                else {
                    return Token(TokenType::Less, { start, pos });
                }
            }

        case '>':
            SkipChar();

            if(value == '>') {
                SkipChar();

                if(value == '=') {
                    SkipChar();
                    return Token(TokenType::RightShiftAssign, { start, pos });
                }
                else {
                    return Token(TokenType::RightShift, { start, pos });
                }
            }
            else {
                if(value == '=') {
                    SkipChar();
                    return Token(TokenType::GreaterEqual, { start, pos });
                }
                else {
                    return Token(TokenType::Greater, { start, pos });
                }
            }

        case '!':
            SkipChar();

            if(value == '=') {
                SkipChar();
                return Token(TokenType::NotEqual, { start, pos });
            }
            else {
                return Token(TokenType::LogicalNot, { start, pos });
            }

        case '&':
            SkipChar();

            if(value == '=') {
                SkipChar();
                return Token(TokenType::BitAndAssign, { start, pos });
            }
            else if(value == '&') {
                SkipChar();
                return Token(TokenType::LogicalAnd, { start, pos });
            }
            else {
                return Token(TokenType::BitAnd, { start, pos });
            }

        case '|':
            SkipChar();

            if(value == '=') {
                SkipChar();
                return Token(TokenType::BitOrAssign, { start, pos });
            }
            else if(value == '|') {
                SkipChar();
                return Token(TokenType::LogicalOr, { start, pos });
            }
            else {
                return Token(TokenType::BitOr, { start, pos });
            }

        case '~':
            SkipChar();

            if(value == '=') {
                SkipChar();
                return Token(TokenType::BitNotAssign, { start, pos });
            }
            else {
                return Token(TokenType::BitNot, { start, pos });
            }

        case '^':
            SkipChar();

            if(value == '=') {
                SkipChar();
                return Token(TokenType::BitXorAssign, { start, pos });
            }
            else {
                return Token(TokenType::BitXor, { start, pos });
            }

        case '?':
            SkipChar();
            return Token(TokenType::QuestionMark, { start, pos });

        case ',':
            SkipChar();
            return Token(TokenType::Comma, { start, pos });
        case ':':
            SkipChar();
            return Token(TokenType::Colon, { start, pos });
        case ';':
            SkipChar();
            return Token(TokenType::Semicolon, { start, pos });
        case '\"':
            return GetStringToken();
        case '\'':
        {
            SkipChar();
            char32_t charLiteral = value;
            SkipChar();
            SkipChar();
            return Token({ start, pos }, charLiteral);
        }
        case '.':
            if(next == chars.end() || !isdigit(*next)) {
                SkipChar();
                return Token(TokenType::Dot, { start, pos });
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
            return GetIdentifierOrKeywordToken();
        default:
            throw std::runtime_error("unexpected character: "s + (char)value);
        }
    }

    static std::string ReadFile(const std::filesystem::path& path)
    {
        std::ifstream fin(path, std::ios::in | std::ios::binary);

        if (!fin.good())
            throw std::runtime_error("failed to open file: "s + path.string());

        fin.seekg(0, std::ios::end);
        auto sz = (size_t)fin.tellg();
        fin.seekg(0, std::ios::beg);

        if (sz == 0)
            throw std::runtime_error("file is empty: "s + path.string());

        std::string ret;
        ret.resize(sz);
        if(!fin.read(reinterpret_cast<char*>(ret.data()), sz))
            throw std::runtime_error("file read failed: "s + path.string());

        return ret;
    }

    void SkipWhitespace()
    {
        while (pos != chars.end())
        {
            if (value == ' ') // spaces
            {
                ++column;
            }
            else if (value == '\t')
            {
                column += TabLength;
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

    void SkipChar() {
        assert(pos != next);
        pos = next;
        value = (next != chars.end()) ? utf8::next(next, chars.end()) : 0;
    }

    void SkipChars(ptrdiff_t count) {
        assert(pos != next);
        utf8::advance(pos, count, chars.end());
        next = pos;
        value = (next != chars.end()) ? utf8::next(next, chars.end()) : 0;
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
        
        auto start = pos;
        SkipChar();

        std::string str;
        
        while (pos != chars.end())
        {
            if (value == '\"')
            {
                SkipChar();
                return Token({ start, pos }, std::move(str));
            }
            else if (value == '\\')
            {
                SkipChar();

                if (pos == chars.end())
                    throw std::runtime_error("unexpected end of input");

                if (value == U'\"') {
                    utf8::append(U'\"', std::back_inserter(str));
                    SkipChar();
                }
                else if (value == U'\\') {
                    utf8::append(U'\\', std::back_inserter(str));
                    SkipChar();
                }
                else if (value == U'/') {
                    utf8::append(U'/', std::back_inserter(str));
                    SkipChar();
                }
                else if (value == U'r') {
                    utf8::append(U'\r', std::back_inserter(str));
                    SkipChar();
                }
                else if (value == U'n') {
                    utf8::append(U'\n', std::back_inserter(str));
                    SkipChar();
                }
                else if (value == U't') {
                    utf8::append(U'\t', std::back_inserter(str));
                    SkipChar();
                }
                else if (value == U'b') {
                    utf8::append(U'\b', std::back_inserter(str));
                    SkipChar();
                }
                else if (value == U'f') {
                    utf8::append(U'\f', std::back_inserter(str));
                    SkipChar();
                }
                else if (value == 'u')
                {
                    SkipChar();

                    if ((chars.end() - pos) < 4)
                        throw std::runtime_error("unexpected end of input");

                    char hex[4];

                    for (int i = 0; i < 4; ++i)
                    {
                        if (!isxdigit(value))
                            throw std::runtime_error("invalid unicode escape sequence");

                        hex[i] = (char)value;
                        SkipChar();
                    }
                    
                    char32_t charValue = (char32_t)std::strtoul(hex, nullptr, 16);
                    utf8::append(charValue, std::back_inserter(str));
                }
                else
                {
                    // eat this character or throw?
                    utf8::append(value, std::back_inserter(str));
                    SkipChar();
                }
            }
            else
            {
                utf8::append(value, std::back_inserter(str));
                SkipChar();
            }
        }

        assert(pos == chars.end());
        throw std::runtime_error("unexpected end of input");
    }

    Token GetNumberToken()
    {
        char* charsPos = reinterpret_cast<char*>(chars.data()) + (pos - chars.begin());
        char* charsEnd = reinterpret_cast<char*>(chars.data()) + chars.size();

        double value;
        std::from_chars_result ret = std::from_chars(charsPos, charsEnd, value);
        if(ret.ec != std::errc())
            throw std::runtime_error("invalid number");

        auto len = ret.ptr - reinterpret_cast<char*>(&pos[0]);
        std::string_view number(charsPos, charsPos + len);
        
        std::string::iterator begin = pos;
        SkipChars(len);

        if(number.find('.') != std::string_view::npos || value == 'f')
        {
            if(value == 'f')
            {
                SkipChar();
                return Token({ begin, pos }, static_cast<float>(value));
            }
            else
            {
                return Token({ begin, pos }, value);
            }
        }
        else
        {
            if(value == 'u')
            {
                SkipChar();
                
                uint64_t uintValue;
                ret = std::from_chars(charsPos, charsEnd, uintValue);
                if(ret.ec != std::errc())
                    throw std::runtime_error("invalid number");

                return Token({ begin, pos }, uintValue);
            }
            else
            {
                int64_t intValue;
                ret = std::from_chars(charsPos, charsEnd, intValue);
                if(ret.ec != std::errc())
                    throw std::runtime_error("invalid number");

                return Token({ begin, pos }, intValue);
            }
        }
    }

    Token GetIdentifierOrKeywordToken()
    {
        assert(isalpha(value) || value == '_');

        std::string::iterator start = pos;
        
        do {
            SkipChar();
        } while (pos != chars.end() && (isalnum(value) || value == '_'));

        std::string_view chars { start, pos };

        if(auto keyword = Keywords.find(std::string(chars)); keyword != Keywords.end())
        {
            return Token(chars, keyword->second);
        }
        else
        {
            return Token(TokenType::Identifier, chars);
        }
    }
};
