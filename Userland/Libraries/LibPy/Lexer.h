/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "LibPy/Token.h"
#include <AK/StringView.h>
#include <AK/Vector.h>
#include <tuple>

namespace Py {

    typedef enum {
        Variable,
        Function,
        Class
    } IdType;

class Lexer {
public:
    Lexer(const StringView&);

    Vector<Token> lex();
    Vector<std::tuple<StringView,IdType> > idList();



private:
    char peek(size_t offset = 0) const;
    char consume();

    Vector<std::tuple<StringView,IdType> > m_idNameList;
    StringView m_input;
    size_t m_index { 0 };
    Position m_previous_position { 0, 0 };
    Position m_position { 0, 0 };
    
};

}
