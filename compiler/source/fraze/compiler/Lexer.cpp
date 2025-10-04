/*--------------------------------------------------------------*
*  Copyright (c) 2025 Nicolas Jinchereau. All rights reserved.  *
*---------------------------------------------------------------*/

#include <fraze/compiler/Lexer.h>
#include <utf8.h>

namespace fraze {

Lexer::Lexer(const std::filesystem::path& sourceFile)
    : filePath(sourceFile.string())
{
    chars = utility::ReadFile(sourceFile);
    pos = chars.begin();
    next = pos;
    value = !chars.empty() ? utf8::next(next, chars.end()) : 0;
    location.file = shared_string(filePath);
    location.lineText = shared_string(std::string_view(chars.begin(), std::find(chars.begin(), chars.end(), '\n')));
}

Lexer::Lexer(const std::filesystem::path& sourceFile, const std::string& mixinCode, size_t mixinLineNumber, bool allowInternalSymbols)
    : filePath(sourceFile.string()), allowInternalSymbols(allowInternalSymbols)
{
    chars = mixinCode;
    pos = chars.begin();
    next = pos;
    value = utf8::next(next, chars.end());
    location.file = shared_string(filePath);
    location.lineText = shared_string(std::string_view(chars.begin(), std::find(chars.begin(), chars.end(), '\n')));
    location.line = mixinLineNumber;
    staticLocation = true;
}

std::vector<Token> Lexer::Tokenize()
{
    std::vector<Token> tokens;
    Tokenize(tokens);
    return tokens;
}

void Lexer::Tokenize(std::vector<Token>& outTokens)
{
    outTokens.clear();

    Token tok;

    do {
        tok = GetNextToken();
        outTokens.push_back(tok);
    } while (!tok.IsType(TokenType::EndOfFile));
}

const std::string& Lexer::GetTokenName(TokenType type)
{
    auto it = TokenNames.find(type);
    assert(it != TokenNames.end());
    return it->second;
}

const std::string& Lexer::GetKeywordName(Keyword keyword)
{
    auto it = KeywordNames.find(keyword);
    assert(it != KeywordNames.end());
    return it->second;
}

Token Lexer::GetNextToken()
{
    SkipWhitespace();

    if (pos == chars.end())
        return Token(location);

    auto start = pos;
    auto loc = location;

    switch (value)
    {
    case '{':
        SkipChar();
        return Token(loc, TokenType::LeftBrace);
    case '}':
        SkipChar();
        return Token(loc, TokenType::RightBrace);
    case '[':
        SkipChar();
        return Token(loc, TokenType::LeftBracket);
    case ']':
        SkipChar();
        return Token(loc, TokenType::RightBracket);
    case '(':
        SkipChar();
        return Token(loc, TokenType::LeftParen);
    case ')':
        SkipChar();
        return Token(loc, TokenType::RightParen);
    case '=':
        SkipChar();

        if(value == '=') {
            SkipChar();
            return Token(loc, TokenType::Equal);
        }
        else {
            return Token(loc, TokenType::Assign);
        }

    case '+':
        SkipChar();

        if(value == '=') {
            SkipChar();
            return Token(loc, TokenType::AddAssign);
        }
        else if(value == '+') {
            SkipChar();
            return Token(loc, TokenType::Increment);
        }
        else {
            return Token(loc, TokenType::Add);
        }

    case '-':
        SkipChar();

        if(value == '=') {
            SkipChar();
            return Token(loc, TokenType::SubAssign);
        }
        else if(value == '-') {
            SkipChar();
            return Token(loc, TokenType::Decrement);
        }
        else {
            return Token(loc, TokenType::Sub);
        }

    case '*':
        SkipChar();

        if(value == '=') {
            SkipChar();
            return Token(loc, TokenType::MulAssign);
        }
        else {
            return Token(loc, TokenType::Mul);
        }

    case '/':
        SkipChar();

        if(value == '=') {
            SkipChar();
            return Token(loc, TokenType::DivAssign);
        }
        else if(value == '/') {
            SkipChar();

            while(value != '\n')
                SkipChar();

            auto end = pos;
            SkipChar();
            return Token(loc, TokenType::LineComment);
        }
        else if(value == '*') {
            SkipChar();

            do {
                while(value != '*') {
                    SkipChar();
                }

                SkipChar();

            } while(value != '/');

            SkipChar();
            return Token(loc, TokenType::BlockComment);
        }
        else {
            return Token(loc, TokenType::Div);
        }

    case '%':
        SkipChar();

        if(value == '=') {
            SkipChar();
            return Token(loc, TokenType::ModAssign);
        }
        else {
            return Token(loc, TokenType::Mod);
        }

    case '<':
        SkipChar();

        if(value == '<') {
            SkipChar();

            if(value == '=') {
                SkipChar();
                return Token(loc, TokenType::LeftShiftAssign);
            }
            else {
                return Token(loc, TokenType::LeftShift);
            }
        }
        else {
            if(value == '=') {
                SkipChar();
                return Token(loc, TokenType::LessEqual);
            }
            else {
                return Token(loc, TokenType::Less);
            }
        }

    case '>':
        // handling of >, >= >>, >>= are context-dependent and handled in the parser
        SkipChar();
        return Token(loc, TokenType::Greater);

    case '!':
        SkipChar();

        if(value == '=') {
            SkipChar();
            return Token(loc, TokenType::NotEqual);
        }
        else {
            return Token(loc, TokenType::LogicalNot);
        }

    case '&':
        SkipChar();

        if(value == '=') {
            SkipChar();
            return Token(loc, TokenType::BitAndAssign);
        }
        else if(value == '&') {
            SkipChar();
            return Token(loc, TokenType::LogicalAnd);
        }
        else if(value == '?') {
            SkipChar();
            return Token(loc, TokenType::BitTest);
        }
        else {
            return Token(loc, TokenType::BitAnd);
        }

    case '|':
        SkipChar();

        if(value == '=') {
            SkipChar();
            return Token(loc, TokenType::BitOrAssign);
        }
        else if(value == '|') {
            SkipChar();
            return Token(loc, TokenType::LogicalOr);
        }
        else {
            return Token(loc, TokenType::BitOr);
        }

    case '~':
        SkipChar();
        return Token(loc, TokenType::BitNot);

    case '^':
        SkipChar();

        if(value == '=') {
            SkipChar();
            return Token(loc, TokenType::BitXorAssign);
        }
        else {
            return Token(loc, TokenType::BitXor);
        }

    case '?':
        SkipChar();
        return Token(loc, TokenType::QuestionMark);

    case ',':
        SkipChar();
        return Token(loc, TokenType::Comma);
    case ':':
        SkipChar();
        return Token(loc, TokenType::Colon);
    case ';':
        SkipChar();
        return Token(loc, TokenType::Semicolon);
    case '\"':
        return GetStringToken();
    case '.':
        if(next == chars.end() || !isdigit(*next)) {
            SkipChar();
            return Token(loc, TokenType::Dot);
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
    case '$':
        if(allowInternalSymbols)
            return GetIdentifierOrKeywordToken();
        [[fallthrough]];
    default:
        ENFORCE(false, location, "unexpected character: {}", (char)value);
    }
}

void Lexer::SkipWhitespace()
{
    while (pos != chars.end())
    {
        if (value == ' ') // spaces
        {
            if(!staticLocation)
                ++location.column;
        }
        else if (value == '\t')
        {
            if(!staticLocation)
                location.column += TabLength;
        }
        else if (value == '\n') // new line
        {
            if(!staticLocation)
            {
                ++location.line;
                location.column = 1;
            }

            location.lineText = shared_string(std::string_view(next, std::find(next, chars.end(), '\n')));
        }
        else if (value == '\r' || value == '\v' || value == '\f')
        {
            // ignore
        }
        else
        {
            // found non-whitespace character
            break;
        }

        pos = next;
        value = (next != chars.end()) ? utf8::next(next, chars.end()) : 0;
    }
}


void Lexer::SkipChar()
{
    assert(pos != next);

    if (value == '\t')
    {
        if(!staticLocation)
            location.column += TabLength;
    }
    else if (value == '\n')
    {
        if(!staticLocation)
        {
            ++location.line;
            location.column = 1;
        }

        location.lineText = shared_string(std::string_view(next, std::find(next, chars.end(), '\n')));
    }
    else // spaces or non-whitespace
    {
        if(!staticLocation)
            ++location.column;
    }

    pos = next;
    value = (next != chars.end()) ? utf8::next(next, chars.end()) : 0;
}

void Lexer::SkipChars(ptrdiff_t count)
{
    assert(pos != next);
    utf8::advance(pos, count, chars.end());
    next = pos;
    value = (next != chars.end()) ? utf8::next(next, chars.end()) : 0;

    if(!staticLocation)
        location.column += count;
}

char32_t Lexer::PeekNext()
{
    auto peek = next;
    return (peek != chars.end()) ? utf8::next(peek, chars.end()) : 0;
}

bool Lexer::IsStartOfNumber(char32_t c) {
    return (c >= '0' && c <= '9') || c == '-' || c == '+';
}

bool Lexer::IsStartOf(char32_t c) {
    return (c >= '0' && c <= '9') || c == '-' || c == '+';
}

Token Lexer::GetStringToken()
{
    assert(value == '\"');

    auto start = pos;
    auto loc = location;

    SkipChar();

    std::string str;

    while (pos != chars.end())
    {
        if (value == '\"')
        {
            SkipChar();
            return Token(loc, shared_string(std::move(str)), TokenType::StringLiteral);
        }
        else if (value == '\\')
        {
            SkipChar();

            ENFORCE(pos != chars.end(), location, "unexpected end of input");

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

                ENFORCE((chars.end() - pos) >= 4, location, "unexpected end of input");

                char hex[4];

                for (int i = 0; i < 4; ++i)
                {
                    ENFORCE(isxdigit(value), location, "invalid unicode escape sequence");
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
    ENFORCE(false, location, "unexpected end of input");
}

Token Lexer::GetNumberToken()
{
    int base = 10;

    if(value == '0')
    {
        auto nextVal = PeekNext();
        if( nextVal == 'x' || nextVal == 'X')
        {
            base = 16;
            SkipChars(2);
        }
        else if( nextVal == 'b' || nextVal == 'B')
        {
            base = 2;
            SkipChars(2);
        }
    }

    char* charsPos = reinterpret_cast<char*>(chars.data()) + (pos - chars.begin());
    char* charsEnd = reinterpret_cast<char*>(chars.data()) + chars.size();

    if(base == 10)
    {
        double numberValue;
        std::from_chars_result ret = std::from_chars(charsPos, charsEnd, numberValue);
        ENFORCE(ret.ec == std::errc(), location, "invalid number");

        auto len = ret.ptr - reinterpret_cast<char*>(&pos[0]);
        std::string_view number(charsPos, charsPos + len);

        auto start = pos;
        auto loc = location;

        SkipChars(len);

        if(number.contains("."))
        {
            return Token(loc, numberValue);
        }
        else
        {
            int64_t integerValue;
            std::from_chars_result ret = std::from_chars(charsPos, charsEnd, integerValue);
            assert(ret.ec == std::errc());
            return Token(loc, integerValue);
        }
    }
    else
    {
        auto loc = location;
        auto start = pos;

        uint64_t unsignedIntegerValue;
        std::from_chars_result ret = std::from_chars(charsPos, charsEnd, unsignedIntegerValue, base);
        ENFORCE(ret.ec == std::errc(), location, "invalid number");

        int64_t integerValue = static_cast<int64_t>(unsignedIntegerValue);

        auto len = ret.ptr - reinterpret_cast<char*>(&pos[0]);
        SkipChars(len);

        return Token(loc, integerValue);
    }
}

Token Lexer::GetIdentifierOrKeywordToken()
{
    assert(isalpha(value) || value == '_' || (value == '$' && allowInternalSymbols));

    auto start = pos;
    auto loc = location;

    do {
        SkipChar();
    } while (pos != chars.end() && (isalnum(value) || value == '_' || (value == '$' && allowInternalSymbols)));

    std::string_view chars { start, pos };
    return Token(loc, shared_string(chars), TokenType::Identifier);
}

} // fraze
