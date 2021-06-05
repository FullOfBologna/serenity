set(SOURCES
    AST.cpp
    Lexer.cpp
    Parser.cpp
    Preprocessor.cpp
    SyntaxHighlighter.cpp
    Token.cpp
)

serenity_lib(LibPy py)
target_link_libraries(LibPy LibCore LibSyntax)
