/*
 * Copyright (c) 2020, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Debug.h>
#include <LibPy/Lexer.h>
#include <LibPy/SyntaxHighlighter.h>
#include <LibGUI/TextEditor.h>
#include <LibGfx/Font.h>
#include <LibGfx/Palette.h>

namespace Py {

static Syntax::TextStyle style_for_token_type(Gfx::Palette const& palette, Py::Token::Type type)
{
    switch (type) {
    case Py::Token::Type::Keyword:
        return { palette.syntax_keyword(), true };
    case Py::Token::Type::KnownType:
        return { palette.syntax_type(), true };
    // case Py::Token::Type::FunctionDefine:
    //     return { palette.syntax_keyword(), true};
    case Py::Token::Type::DoubleQuotedString:
    case Py::Token::Type::SingleQuotedString:
    case Py::Token::Type::RawString:
        return { palette.syntax_string(), false };
    case Py::Token::Type::Integer:
    case Py::Token::Type::Float:
        return { palette.syntax_number(), false };
    case Py::Token::Type::ImportModule:
        return { palette.syntax_preprocessor_value(), false };
    case Py::Token::Type::EscapeSequence:
        return { palette.syntax_keyword(), true };
    case Py::Token::Type::ImportStatement:
        return { palette.syntax_preprocessor_statement(), false };
    case Py::Token::Type::Comment:
        return { palette.syntax_comment(), false };
    default:
        return { palette.base_text(), false };
    }
}

static Syntax::TextStyle style_for_token_type(Gfx::Palette const& palette, Py::Token::Type type, Py::IdType idType)
{

    switch (type) {
    case Py::Token::Type::Identifier:
        if(idType == IdType::Function){
            dbgln_if(SYNTAX_HIGHLIGHTING_DEBUG, "idType : Function");

            return {palette.syntax_function_identifier(),true};
        }else if(idType == IdType::Class){
            dbgln_if(SYNTAX_HIGHLIGHTING_DEBUG, "idType : Class");

            return { palette.syntax_class_identifier(), true };
        } else {
            return { palette.syntax_identifier(), false };
        }
    default:
        return { palette.base_text(), false };
    }
}
bool SyntaxHighlighter::is_identifier(u64 token) const
{
    auto Py_token = static_cast<Py::Token::Type>(token);
    return Py_token == Py::Token::Type::Identifier;
}

bool SyntaxHighlighter::is_navigatable(u64 token) const
{
    auto Py_token = static_cast<Py::Token::Type>(token);
    return Py_token == Py::Token::Type::ImportModule;
}

void SyntaxHighlighter::rehighlight(Palette const& palette)
{
    auto text = m_client->get_text();
    Py::Lexer lexer(text);
    auto tokens = lexer.lex();
    auto idList = lexer.idList();

    uint32_t identifierIndex = 0;

    Vector<GUI::TextDocumentSpan> spans;
    for (auto& token : tokens) {
        // FIXME: The +1 for the token end column is a quick hack due to not wanting to modify the lexer (which is also used by the parser). Maybe there's a better way to do this.
        GUI::TextDocumentSpan span;
        span.range.set_start({ token.start().line, token.start().column });
        span.range.set_end({ token.end().line, token.end().column + 1 });

        if(token.type() == Token::Type::Identifier) {
            // FIXME: If a new token is seen, don't add the token to the token list.
            dbgln_if(SYNTAX_HIGHLIGHTING_DEBUG, "Token : {}", token.text());
            auto style = style_for_token_type(palette, token.type(), std::get<1>(idList[identifierIndex]));
            span.attributes.color = style.color;
            span.attributes.bold = style.bold;
            span.is_skippable = token.type() == Py::Token::Type::Whitespace;
            span.data = static_cast<u64>(token.type());
            spans.append(span);
            identifierIndex++;
        } else {
            auto style = style_for_token_type(palette, token.type());
            span.attributes.color = style.color;
            span.attributes.bold = style.bold;
            span.is_skippable = token.type() == Py::Token::Type::Whitespace;
            span.data = static_cast<u64>(token.type());
            spans.append(span);
        }
    }

    m_client->do_set_spans(move(spans));

    m_has_brace_buddies = false;
    highlight_matching_token_pair();

    m_client->do_update();
}

Vector<SyntaxHighlighter::MatchingTokenPair> SyntaxHighlighter::matching_token_pairs_impl() const
{
    static Vector<SyntaxHighlighter::MatchingTokenPair> pairs;
    if (pairs.is_empty()) {
        pairs.append({ static_cast<u64>(Py::Token::Type::LeftCurly), static_cast<u64>(Py::Token::Type::RightCurly) });
        pairs.append({ static_cast<u64>(Py::Token::Type::LeftParen), static_cast<u64>(Py::Token::Type::RightParen) });
        pairs.append({ static_cast<u64>(Py::Token::Type::LeftBracket), static_cast<u64>(Py::Token::Type::RightBracket) });
    }
    return pairs;
}

bool SyntaxHighlighter::token_types_equal(u64 token1, u64 token2) const
{
    return static_cast<Py::Token::Type>(token1) == static_cast<Py::Token::Type>(token2);
}

SyntaxHighlighter::~SyntaxHighlighter()
{
}

}
