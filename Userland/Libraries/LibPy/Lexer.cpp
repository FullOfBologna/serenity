/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "Lexer.h"
#include <AK/HashTable.h>
#include <AK/StdLibExtras.h>
#include <AK/String.h>
#include <ctype.h>

//DEBUG Includes
#include <AK/Debug.h>
#include <AK/SourceLocation.h>

namespace Py {

Lexer::Lexer(const StringView& input)
    : m_defTokenList(),
    m_classTokenList(),
    m_idNameList(),
    m_input(input)
{
}

char Lexer::peek(size_t offset) const
{
    if ((m_index + offset) >= m_input.length())
        return 0;
    return m_input[m_index + offset];
}

char Lexer::consume()
{
    VERIFY(m_index < m_input.length());
    char ch = m_input[m_index++];
    m_previous_position = m_position;
    if (ch == '\n') {
        m_position.line++;
        m_position.column = 0;
    } else {
        m_position.column++;
    }
    return ch;
}

static bool is_valid_first_character_of_identifier(char ch)
{
    return isalpha(ch) || ch == '_' || ch == '$';
}

static bool is_valid_nonfirst_character_of_identifier(char ch)
{
    return is_valid_first_character_of_identifier(ch) || isdigit(ch);
}

constexpr const char* s_known_keywords[] = {
    "and",
    "as",   
    "assert",
    "break",
    "class",
    "continue",
    "def",
    "del",
    "elif",
    "else",
    "except",
    "finally",
    "for",
    "from",
    "global",
    "if",
    "import",
    "in",
    "is",
    "lambda",
    "None",
    "nonlocal",
    "not",
    "or",
    "pass",
    "raise",
    "return",
    "True",
    "try",
    "while",
    "with",
    "yield"
};

// constexpr const char* s_known_types[] = {

// };

static bool is_keyword(const StringView& string)
{
    static HashTable<String> keywords(array_size(s_known_keywords));
    if (keywords.is_empty()) {
        keywords.set_from(s_known_keywords);
    }
    return keywords.contains(string);
}

// static bool is_namedVariable(const StringView& string)
// {
//     static HashTable<String> variables(array_size())
// }

// static bool is_known_type(const StringView& string)
// {
//     static HashTable<String> types(array_size(s_known_types));
//     if (types.is_empty()) {
//         types.set_from(s_known_types);
//     }
//     return types.contains(string);
// }

Vector<Token> Lexer::lex()
{
    Vector<Token> tokens;

    size_t token_start_index = 0;
    Position token_start_position;

    auto emit_single_char_token = [&](auto type) {
        tokens.empend(type, m_position, m_position, m_input.substring_view(m_index, 1));
        consume();
    };

    auto begin_token = [&] {
        token_start_index = m_index;
        token_start_position = m_position;
    };
    auto commit_token = [&](auto type) {
        tokens.empend(type, token_start_position, m_previous_position, m_input.substring_view(token_start_index, m_index - token_start_index));
    };

    auto emit_token_equals = [&](auto type, auto equals_type) {
        if (peek(1) == '=') {
            begin_token();
            consume();
            consume();
            commit_token(equals_type);
            return;
        }
        emit_single_char_token(type);
    };

    auto match_escape_sequence = [&]() -> size_t {
        switch (peek(1)) {
        case '\'':
        case '"':
        case '?':
        case '\\':
        case 'a':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'v':
            return 2;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7': {
            size_t octal_digits = 1;
            for (size_t i = 0; i < 2; ++i) {
                char next = peek(2 + i);
                if (next < '0' || next > '7')
                    break;
                ++octal_digits;
            }
            return 1 + octal_digits;
        }
        case 'x': {
            size_t hex_digits = 0;
            while (isxdigit(peek(2 + hex_digits)))
                ++hex_digits;
            return 2 + hex_digits;
        }
        case 'u':
        case 'U': {
            bool is_unicode = true;
            size_t number_of_digits = peek(1) == 'u' ? 4 : 8;
            for (size_t i = 0; i < number_of_digits; ++i) {
                if (!isxdigit(peek(2 + i))) {
                    is_unicode = false;
                    break;
                }
            }
            return is_unicode ? 2 + number_of_digits : 0;
        }
        default:
            return 0;
        }
    };

    auto match_string_prefix = [&](char quote) -> size_t {
        if (peek() == quote)
            return 1;
        if (peek() == 'L' && peek(1) == quote)
            return 2;
        if (peek() == 'u') {
            if (peek(1) == quote)
                return 2;
            if (peek(1) == '8' && peek(2) == quote)
                return 3;
        }
        if (peek() == 'U' && peek(1) == quote)
            return 2;
        return 0;
    };

    while (m_index < m_input.length()) {
        auto ch = peek();
        if (isspace(ch)) {
            begin_token();
            while (isspace(peek()))
                consume();
            commit_token(Token::Type::Whitespace);
            continue;
        }
        // if (ch == 'd') {
        //     begin_token();
        //     consume();
        //     if(peek() == 'e'){
        //         consume();
        //         if(peek() == 'f'){
        //             consume();
        //             if(isspace(peek())){
        //                 commit_token(Token::Type::Keyword);
        //                 continue;
        //             }
        //         }
        //     }
        // }
        if (ch == '(') {
            emit_single_char_token(Token::Type::LeftParen);
            continue;
        }
        if (ch == ')') {
            emit_single_char_token(Token::Type::RightParen);
            continue;
        }
        if (ch == '{') {
            emit_single_char_token(Token::Type::LeftCurly);
            continue;
        }
        if (ch == '}') {
            emit_single_char_token(Token::Type::RightCurly);
            continue;
        }
        if (ch == '[') {
            emit_single_char_token(Token::Type::LeftBracket);
            continue;
        }
        if (ch == ']') {
            emit_single_char_token(Token::Type::RightBracket);
            continue;
        }
        if (ch == '<') {
            begin_token();
            consume();
            if (peek() == '<') {
                consume();
                if (peek() == '=') {
                    consume();
                    commit_token(Token::Type::LessLessEquals);
                    continue;
                }
                commit_token(Token::Type::LessLess);
                continue;
            }
            if (peek() == '=') {
                consume();
                commit_token(Token::Type::LessEquals);
                continue;
            }
            if (peek() == '>') {
                consume();
                commit_token(Token::Type::LessGreater);
                continue;
            }
            commit_token(Token::Type::Less);
            continue;
        }
        if (ch == '>') {
            begin_token();
            consume();
            if (peek() == '>') {
                consume();
                if (peek() == '=') {
                    consume();
                    commit_token(Token::Type::GreaterGreaterEquals);
                    continue;
                }
                commit_token(Token::Type::GreaterGreater);
                continue;
            }
            if (peek() == '=') {
                consume();
                commit_token(Token::Type::GreaterEquals);
                continue;
            }
            commit_token(Token::Type::Greater);
            continue;
        }
        if (ch == ',') {
            emit_single_char_token(Token::Type::Comma);
            continue;
        }
        if (ch == '+') {
            begin_token();
            consume();
            // if (peek() == '+') {
            //     consume();
            //     commit_token(Token::Type::PlusPlus);
            //     continue;
            // }
            if (peek() == '=') {
                consume();
                commit_token(Token::Type::PlusEquals);
                continue;
            }
            commit_token(Token::Type::Plus);
            continue;
        }
        if (ch == '-') {
            begin_token();
            consume();
            if (peek() == '=') {
                consume();
                commit_token(Token::Type::MinusEquals);
                continue;
            }
            if (peek() == '>') {
                consume();
                if (peek() == '*') {
                    consume();
                    commit_token(Token::Type::ArrowAsterisk);
                    continue;
                }
                commit_token(Token::Type::Arrow);
                continue;
            }
            commit_token(Token::Type::Minus);
            continue;
        }
        if (ch == '*') {
            emit_token_equals(Token::Type::Asterisk, Token::Type::AsteriskEquals);
            continue;
        }
        if (ch == '%') {
            emit_token_equals(Token::Type::Percent, Token::Type::PercentEquals);
            continue;
        }
        if (ch == '^') {
            emit_token_equals(Token::Type::Caret, Token::Type::CaretEquals);
            continue;
        }
        if (ch == '!') {
            emit_token_equals(Token::Type::ExclamationMark, Token::Type::ExclamationMarkEquals);
            continue;
        }
        if (ch == '=') {
            emit_token_equals(Token::Type::Equals, Token::Type::EqualsEquals);
            continue;
        }
        if (ch == '&') {
            begin_token();
            consume();
            if (peek() == '&') {
                consume();
                commit_token(Token::Type::AndAnd);
                continue;
            }
            if (peek() == '=') {
                consume();
                commit_token(Token::Type::AndEquals);
                continue;
            }
            commit_token(Token::Type::And);
            continue;
        }
        if (ch == '|') {
            begin_token();
            consume();
            if (peek() == '|') {
                consume();
                commit_token(Token::Type::PipePipe);
                continue;
            }
            if (peek() == '=') {
                consume();
                commit_token(Token::Type::PipeEquals);
                continue;
            }
            commit_token(Token::Type::Pipe);
            continue;
        }
        if (ch == '~') {
            emit_single_char_token(Token::Type::Tilde);
            continue;
        }
        if (ch == '?') {
            emit_single_char_token(Token::Type::QuestionMark);
            continue;
        }
        if (ch == ':') {
            begin_token();
            consume();
            if (peek() == ':') {
                consume();
                if (peek() == '*') {
                    consume();
                    commit_token(Token::Type::ColonColonAsterisk);
                    continue;
                }
                commit_token(Token::Type::ColonColon);
                continue;
            }
            commit_token(Token::Type::Colon);
            continue;
        }
        if (ch == ';') {
            emit_single_char_token(Token::Type::Semicolon);
            continue;
        }
        if (ch == '.') {
            begin_token();
            consume();
            if (peek() == '*') {
                consume();
                commit_token(Token::Type::DotAsterisk);
                continue;
            }
            commit_token(Token::Type::Dot);
            continue;
        }
        if (ch == '#') {
            begin_token();
            consume();

            if (is_valid_first_character_of_identifier(peek()))
                while (peek() && is_valid_nonfirst_character_of_identifier(peek()))
                    consume();

            auto directive = StringView(m_input.characters_without_null_termination() + token_start_index, m_index - token_start_index);
            if (directive == "import") {
                commit_token(Token::Type::ImportStatement);

                begin_token();
                while (isspace(peek())) {
                    consume();
                    commit_token(Token::Type::ImportModule);
                }

                commit_token(Token::Type::Whitespace);

                begin_token();

            } 
            continue;
        }
        if (ch == '/' && peek(1) == '/') {
            begin_token();
            while (peek() && peek() != '\n')
                consume();
            commit_token(Token::Type::Comment);
            continue;
        }
        if (ch == '/' && peek(1) == '*') {
            begin_token();
            consume();
            consume();
            bool comment_block_ends = false;
            while (peek()) {
                if (peek() == '*' && peek(1) == '/') {
                    comment_block_ends = true;
                    break;
                }

                consume();
            }

            if (comment_block_ends) {
                consume();
                consume();
            }

            commit_token(Token::Type::Comment);
            continue;
        }
        if (ch == '/') {
            emit_token_equals(Token::Type::Slash, Token::Type::SlashEquals);
            continue;
        }
        if (size_t prefix = match_string_prefix('"'); prefix > 0) {
            begin_token();
            for (size_t i = 0; i < prefix; ++i)
                consume();
            while (peek()) {
                if (peek() == '\\') {
                    if (size_t escape = match_escape_sequence(); escape > 0) {
                        commit_token(Token::Type::DoubleQuotedString);
                        begin_token();
                        for (size_t i = 0; i < escape; ++i)
                            consume();
                        commit_token(Token::Type::EscapeSequence);
                        begin_token();
                        continue;
                    }
                }

                // If string is not terminated - stop before EOF
                if (!peek(1))
                    break;

                if (consume() == '"')
                    break;
            }
            commit_token(Token::Type::DoubleQuotedString);
            continue;
        }
        if (size_t prefix = match_string_prefix('R'); prefix > 0 && peek(prefix) == '"') {
            begin_token();
            for (size_t i = 0; i < prefix + 1; ++i)
                consume();
            size_t prefix_start = m_index;
            while (peek() && peek() != '(')
                consume();
            StringView prefix_string = m_input.substring_view(prefix_start, m_index - prefix_start);
            while (peek()) {
                if (consume() == '"') {
                    VERIFY(m_index >= prefix_string.length() + 2);
                    VERIFY(m_input[m_index - 1] == '"');
                    if (m_input[m_index - 1 - prefix_string.length() - 1] == ')') {
                        StringView suffix_string = m_input.substring_view(m_index - 1 - prefix_string.length(), prefix_string.length());
                        if (prefix_string == suffix_string)
                            break;
                    }
                }
            }
            commit_token(Token::Type::RawString);
            continue;
        }
        if (size_t prefix = match_string_prefix('\''); prefix > 0) {
            begin_token();
            for (size_t i = 0; i < prefix; ++i)
                consume();
            while (peek()) {
                if (peek() == '\\') {
                    if (size_t escape = match_escape_sequence(); escape > 0) {
                        commit_token(Token::Type::SingleQuotedString);
                        begin_token();
                        for (size_t i = 0; i < escape; ++i)
                            consume();
                        commit_token(Token::Type::EscapeSequence);
                        begin_token();
                        continue;
                    }
                }

                if (consume() == '\'')
                    break;
            }
            commit_token(Token::Type::SingleQuotedString);
            continue;
        }
        if (isdigit(ch) || (ch == '.' && isdigit(peek(1)))) {
            begin_token();
            consume();

            auto type = ch == '.' ? Token::Type::Float : Token::Type::Integer;
            bool is_hex = false;
            bool is_binary = false;

            auto match_exponent = [&]() -> size_t {
                char ch = peek();
                if (ch != 'e' && ch != 'E' && ch != 'p' && ch != 'P')
                    return 0;

                type = Token::Type::Float;
                size_t length = 1;
                ch = peek(length);
                if (ch == '+' || ch == '-') {
                    ++length;
                }
                for (ch = peek(length); isdigit(ch); ch = peek(length)) {
                    ++length;
                }
                return length;
            };

            auto match_type_literal = [&]() -> size_t {
                size_t length = 0;
                for (;;) {
                    char ch = peek(length);
                    if ((ch == 'u' || ch == 'U') && type == Token::Type::Integer) {
                        ++length;
                    } else if ((ch == 'f' || ch == 'F') && !is_binary) {
                        type = Token::Type::Float;
                        ++length;
                    } else if (ch == 'l' || ch == 'L') {
                        ++length;
                    } else
                        return length;
                }
            };

            if (peek() == 'b' || peek() == 'B') {
                consume();
                is_binary = true;
                for (char ch = peek(); ch == '0' || ch == '1' || (ch == '\'' && peek(1) != '\''); ch = peek()) {
                    consume();
                }
            } else {
                if (peek() == 'x' || peek() == 'X') {
                    consume();
                    is_hex = true;
                }

                for (char ch = peek(); (is_hex ? isxdigit(ch) : isdigit(ch)) || (ch == '\'' && peek(1) != '\'') || ch == '.'; ch = peek()) {
                    if (ch == '.') {
                        if (type == Token::Type::Integer) {
                            type = Token::Type::Float;
                        } else
                            break;
                    };
                    consume();
                }
            }

            if (!is_binary) {
                size_t length = match_exponent();
                for (size_t i = 0; i < length; ++i)
                    consume();
            }

            size_t length = match_type_literal();
            for (size_t i = 0; i < length; ++i)
                consume();

            commit_token(type);
            continue;
        }
        if (is_valid_first_character_of_identifier(ch)) {
            begin_token();
            while (peek() && is_valid_nonfirst_character_of_identifier(peek()))
                consume();
            auto token_view = StringView(m_input.characters_without_null_termination() + token_start_index, m_index - token_start_index);
            
            if (is_keyword(token_view))
            {
                commit_token(Token::Type::Keyword);
                if(token_view.equals_ignoring_case("def"))
                {

                    m_defTokenList.append(tokens.size()-1);
                    // dbgln_if(PY_DEBUG, "{}: Def found! m_defTokenList: {}", SourceLocation::current(), m_defTokenList[tokens.size()-1]);

                }

                if(token_view.equals_ignoring_case("class"))
                {
                    m_classTokenList.append(tokens.size()-1);
                }
            }
            // else if (is_known_type(token_view))
            //     commit_token(Token::Type::KnownType);
            else
            {

                //Right here parse the identifier into either Variables, Modules, and Functions.
                // -2 to account for the whitespace between the def and the function name. 
                size_t prevTokenIndex = tokens.size()-2;
                // dbgln_if(PY_DEBUG, "{}: prevTokenIndex = {}, tokens.size(): {}",SourceLocation::current(), prevTokenIndex, tokens.size());

                if(prevTokenIndex < tokens.size())
                {

                    //Keyword Token does not contain the value of the Keyword. If we encounter a def or a class keyword, need to store its location in the token list... 
                    
                    // Token prevToken = tokens[prevTokenIndex];
                    commit_token(Token::Type::Identifier);
                    std::tuple<StringView,IdType> tempTuple;
                    bool isVar = true;

                    dbgln_if(PY_DEBUG, "prevToken: {}, token_view = {}, tokens.size(): {}",SourceLocation::current(), token_view, tokens.size());

                    //TODO - Put in a way to check whether the identifier has already been added 
                    //       So the id List can be searched through to identify whether the identifier already exists.
                    // Search through the idNameList for the current token. If already exists skip the next loops. 

                    for(auto id : m_idNameList)
                    {
                        if(token_view == std::get<0>(id))
                        {
                            dbgln_if(PY_DEBUG, "prevToken: {}, token_view = {}, tokens.size(): {}",SourceLocation::current(), token_view, tokens.size());

                            continue;
                        }
                    }

                    for(auto index : m_defTokenList)
                    {
                        if(prevTokenIndex == index)
                        {
                            //We've found the def associated with this function call. 
                            tempTuple = std::make_tuple(token_view, IdType::Function);
                            m_idNameList.append(tempTuple);
                            isVar = false;
                            // dbgln_if(PY_DEBUG, "{}: Function Name: {}, Type: Function", SourceLocation::current(), get<0>(m_idNameList[m_idNameList.size()-1]));
                        }
                    }

                    for(auto index : m_classTokenList)
                    {
                        if(prevTokenIndex == index)
                        {
                            tempTuple = std::make_tuple(token_view, IdType::Class);
                            m_idNameList.append(tempTuple);
                            isVar = false;
                        }
                    }
                    
                    if(isVar)
                    {
                        tempTuple = std::make_tuple(token_view, IdType::Variable);
                        m_idNameList.append(tempTuple);
                    }
                }
            }
            continue;
        }

        if (ch == '\\' && peek(1) == '\n') {
            consume();
            consume();
            continue;
        }

        dbgln("Unimplemented token character: {}", ch);
        emit_single_char_token(Token::Type::Unknown);
    }
    return tokens;
}


Vector<std::tuple<StringView,IdType> > Lexer::idList(){
    return m_idNameList;
};

}
