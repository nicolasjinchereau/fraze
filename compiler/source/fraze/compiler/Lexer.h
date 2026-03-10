/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

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
#include <filesystem>
#include <charconv>
#include <fraze/common/Exception.h>
#include <fraze/common/SharedString.h>
#include <fraze/common/Utility.h>

using namespace std::string_literals;

namespace fraze {

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
    Increment,        //  ++
    Sub,              //  -
    SubAssign,        //  -=
    Decrement,        //  --
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
    BitTest,          //  &?
    BitXor,           //  ^
    BitXorAssign,     //  ^=
    BitNot,           //  ~

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

    IntegerLiteral,   //  1234
    NumberLiteral,    //  12.34
    StringLiteral,    //  "abcd1234"

    Identifier,       //  _asdf3423

    LineComment,      //  // text
    BlockComment      //  /* text */
};

enum class Keyword : uint8_t
{
    // symbol scope
    Section,
    Expose,

    // definitions
    Class,
    Interface,
    Functor,
    Struct,
    Enum,

    // storage classes
    Extern,
    Static,
    Private,

    // statements
    Assert,
    If,
    Else,
    For,
    Return,
    While,
    Goto,
    Code,

    // operators
    As,
    Is,
    New,
    SizeOf,
    Await,
    Fold,
    TypeOf,

    // property accessors
    Get,
    Set,

    // types
    Object,
    Boolean,
    Integer,
    Number,
    String,
    Void,
    
    // functions
    Operator,
    
    // literals
    This,
    True,
    False,
    Null,
};

const string_view_map<const Keyword> Keywords
{
    { "section",   Keyword::Section },
    { "expose",    Keyword::Expose },

    { "class",     Keyword::Class },
    { "interface", Keyword::Interface },
    { "functor",   Keyword::Functor },
    { "struct",    Keyword::Struct },
    { "enum",      Keyword::Enum },

    { "extern",    Keyword::Extern },
    { "static",    Keyword::Static },
    { "private",   Keyword::Private },

    { "assert",    Keyword::Assert },
    { "if",        Keyword::If },
    { "else",      Keyword::Else },
    { "for",       Keyword::For },
    { "return",    Keyword::Return },
    { "while",     Keyword::While },
    { "goto",      Keyword::Goto },
    { "code",      Keyword::Code },

    { "as",        Keyword::As },
    { "is",        Keyword::Is },
    { "new",       Keyword::New },
    { "sizeof",    Keyword::SizeOf },
    { "await",     Keyword::Await },
    { "fold",      Keyword::Fold },
    { "typeof",    Keyword::TypeOf },

    { "get",       Keyword::Get },
    { "set",       Keyword::Set },

    { "object",    Keyword::Object },
    { "bool",      Keyword::Boolean },
    { "int",       Keyword::Integer },
    { "num",       Keyword::Number },
    { "string",    Keyword::String },
    { "void",      Keyword::Void },

    { "operator",  Keyword::Operator },

    { "this",      Keyword::This },
    { "true",      Keyword::True },
    { "false",     Keyword::False },
    { "null",      Keyword::Null }
};

const std::unordered_map<Keyword, const std::string> KeywordNames
{
    { Keyword::Section,   "section" },
    { Keyword::Expose,    "expose" },

    { Keyword::Class,     "class" },
    { Keyword::Interface, "interface" },
    { Keyword::Functor,   "functor" },
    { Keyword::Struct,    "struct" },
    { Keyword::Enum,      "enum" },

    { Keyword::Extern,    "extern" },
    { Keyword::Static,    "static" },
    { Keyword::Private,   "private" },

    { Keyword::Assert,    "assert" },
    { Keyword::If,        "if" },
    { Keyword::Else,      "else" },
    { Keyword::For,       "for" },
    { Keyword::Return,    "return" },
    { Keyword::While,     "while" },
    { Keyword::Goto,      "goto" },
    { Keyword::Code,      "code" },

    { Keyword::As,        "as" },
    { Keyword::Is,        "is" },
    { Keyword::New,       "new" },
    { Keyword::SizeOf,    "sizeof" },
    { Keyword::Await,     "await" },
    { Keyword::Fold,      "fold" },
    { Keyword::TypeOf,    "typeof" },

    { Keyword::Get,       "get" },
    { Keyword::Set,       "set" },

    { Keyword::Object,    "object" },
    { Keyword::Boolean,   "bool" },
    { Keyword::Integer,   "int" },
    { Keyword::Number,    "num" },
    { Keyword::String,    "string" },
    { Keyword::Void,      "void" },

    { Keyword::Operator,  "operator" },

    { Keyword::This,      "this" },
    { Keyword::True,      "true" },
    { Keyword::False,     "false" },
    { Keyword::Null,      "null" }
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
    { TokenType::Increment,        "++" },
    { TokenType::Sub,              "-" },
    { TokenType::SubAssign,        "-=" },
    { TokenType::Decrement,        "--" },
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
    { TokenType::BitTest,          "&?" },
    { TokenType::BitXor,           "^" },
    { TokenType::BitXorAssign,     "^=" },
    { TokenType::BitNot,           "~" },

    { TokenType::LogicalAnd,       "&&" },
    { TokenType::LogicalOr,        "||" },
    { TokenType::LogicalNot,       "!" },
    { TokenType::QuestionMark,     "?" },

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

    { TokenType::IntegerLiteral,   "int" },
    { TokenType::NumberLiteral,    "num" },
    { TokenType::StringLiteral,    "string" },

    { TokenType::Identifier,       "identifier" },

    { TokenType::LineComment,      "line comment" },
    { TokenType::BlockComment,     "block comment" }
};

const std::unordered_set<TokenType> OverloadableBinaryOperators
{
    TokenType::Add,
    TokenType::Sub,
    TokenType::Mul,
    TokenType::Div,
    TokenType::Mod,
    TokenType::LeftShift,
    TokenType::RightShift,
    TokenType::BitOr,
    TokenType::BitAnd,
    TokenType::BitTest,
    TokenType::BitXor,
    TokenType::Equal,
    TokenType::NotEqual,
    TokenType::Less,
    TokenType::LessEqual,
    TokenType::Greater,
    TokenType::GreaterEqual,
};

const std::unordered_set<TokenType> CommutativeBinaryOperators
{
    TokenType::Add,
    TokenType::Mul,
    TokenType::BitOr,
    TokenType::BitAnd,
    TokenType::BitXor,
    TokenType::Equal,
    TokenType::NotEqual,
};

const std::unordered_set<TokenType> OverloadablePrefixOperators
{
    TokenType::Add,
    TokenType::Sub,
    TokenType::Increment,
    TokenType::Decrement,
    TokenType::BitNot,
    TokenType::LogicalNot,
};

const std::unordered_set<TokenType> OverloadablePostfixOperators
{
    TokenType::Increment,
    TokenType::Decrement,
};

const std::unordered_map<TokenType, std::string> OperatorOverloadNames
{
    { TokenType::Add, "operator+" },
    { TokenType::Sub, "operator-" },
    { TokenType::Mul, "operator*" },
    { TokenType::Div, "operator/" },
    { TokenType::Mod, "operator%" },
    { TokenType::LeftShift, "operator<<" },
    { TokenType::RightShift, "operator>>" },
    { TokenType::BitOr, "operator|" },
    { TokenType::BitAnd, "operator&" },
    { TokenType::BitTest, "operator&?" },
    { TokenType::BitXor, "operator^" },
    { TokenType::Equal, "operator==" },
    { TokenType::NotEqual, "operator!=" },
    { TokenType::Less, "operator<" },
    { TokenType::LessEqual, "operator<=" },
    { TokenType::Greater, "operator>" },
    { TokenType::GreaterEqual, "operator>=" },
    { TokenType::Increment, "operator++" },
    { TokenType::Decrement, "operator--" },
    { TokenType::BitNot, "operator~" },
    { TokenType::LogicalNot, "operator!" },
};

struct Token
{
    SourceLocation loc;
    TokenType type = TokenType::EndOfFile;
    std::variant<nullptr_t, int64_t, double, shared_string> value;

    Token() = default;

    Token(SourceLocation loc)
        : loc(loc), type(TokenType::EndOfFile) { }

    Token(SourceLocation loc, TokenType type)
        : loc(loc), type(type), value() { }

    Token(SourceLocation loc, int64_t integerLiteral)
        : loc(loc), type(TokenType::IntegerLiteral), value(integerLiteral) { }

    Token(SourceLocation loc, double numberLiteral)
        : loc(loc), type(TokenType::NumberLiteral), value(numberLiteral) { }

    Token(SourceLocation loc, const shared_string& stringLiteral, TokenType type)
        : loc(loc), type(type), value(stringLiteral) { }

    Token(SourceLocation loc, shared_string&& stringLiteral, TokenType type)
        : loc(loc), type(type), value(std::move(stringLiteral)) { }

    bool IsType(TokenType tokenType) const { return type == tokenType; }
    bool IsInteger() const { return std::holds_alternative<int64_t>(value); }
    bool IsNumber() const { return std::holds_alternative<double>(value); }
    bool IsKeyword() const { return IsIdentifier() && Keywords.contains(std::get<shared_string>(value)); }
    bool IsKeyword(Keyword keyword) const { return IsIdentifier() && std::get<shared_string>(value) == KeywordNames.at(keyword); }
    bool IsString() const { return type == TokenType::StringLiteral; }
    bool IsIdentifier() const { return type == TokenType::Identifier; }
    bool IsBasicType() const { return IsKeyword(Keyword::Object) || IsKeyword(Keyword::Boolean) || IsKeyword(Keyword::Integer) || IsKeyword(Keyword::Number) || IsKeyword(Keyword::String) || IsKeyword(Keyword::Void); }

    int64_t GetInteger() const { return std::get<int64_t>(value); }
    double GetNumber() const { return std::get<double>(value); }
    const shared_string& GetString() const { ENFORCE(type == TokenType::StringLiteral, loc, "not an identifier"); return std::get<shared_string>(value); }
    const shared_string& GetIdentifier() const { ENFORCE(type == TokenType::Identifier, loc, "not an identifier"); return std::get<shared_string>(value); }
};

class Lexer
{
    std::string filePath;
    std::string chars;
    std::string::iterator pos;
    std::string::iterator next;
    char32_t value{};
    SourceLocation location { 1, 1 };
    bool staticLocation = false;
    bool allowInternalSymbols = false;

    constexpr static int TabLength = 4;
public:
    Lexer(const std::filesystem::path& sourceFile);
    Lexer(const std::filesystem::path& sourceFile, const std::string& mixinCode, size_t mixinLineNumber, bool allowInternalSymbols);

    std::vector<Token> Tokenize();
    void Tokenize(std::vector<Token>& outTokens);
    static const std::string& GetTokenName(TokenType type);
    static const std::string& GetKeywordName(Keyword keyword);
    static bool IsInternalSymbol(char32_t c);
private:
    Token GetNextToken();
    void SkipWhitespace();
    void SkipChar();
    void SkipChars(ptrdiff_t count);
    char32_t PeekNext();
    bool IsStartOfNumber(char32_t c);
    bool IsStartOf(char32_t c);
    Token GetStringToken();
    Token GetNumberToken();
    Token GetIdentifierOrKeywordToken();
};

} // fraze
