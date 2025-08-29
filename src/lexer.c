#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include "common.h"
#include "./lexer.h"

typedef struct {
    Token_Kind kind;
    const char *text;
} Literal_Token;

Literal_Token literal_tokens[] = {
    {.text = "(", .kind = TOKEN_OPEN_PAREN},
    {.text = ")", .kind = TOKEN_CLOSE_PAREN},
    {.text = "{", .kind = TOKEN_OPEN_CURLY},
    {.text = "}", .kind = TOKEN_CLOSE_CURLY},
    {.text = ";", .kind = TOKEN_SEMICOLON},
};

#define literal_tokens_count (sizeof(literal_tokens) / sizeof(literal_tokens[0]))

const char *keywords[] = {
    // data types
    "int", "short", "long", "float", "double",
    "char", "wchar_t", "char8_t", "char16_t", "char32_t",
    "int8_t", "int16_t", "int32_t", "int64_t",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t",
    "bool", "void",

    // control flow
    "if", "else", "while", "for",
    "do", "switch", "case", "break",
    "goto", "default", "return", "continue",

    // storage classes
    "const", "auto", "register", "static",
    "extern", "thread_local", "mutable",

    // type modifiers
    "signed", "unsigned", "volatile", "inline",

    // memory management
    "new", "delete",

    // boolean literals
    "false", "true", "nullptr",

    // type information
    "typeid", "typename", "decltype",

    // exception handling
    "try", "catch", "throw",

    // c++ specific
    "class", "struct", "union", "enum",
    "public", "private", "protected", "virtual",
    "friend", "explicit", "operator", "template",
    "namespace", "using", "static_assert", "concept",
    "requires", "consteval", "constexpr", "constinit",

    // alignment
    "alignas", "alignof",

    // coroutines
    "co_await", "co_return", "co_yield",

    // casting
    "dynamic_cast", "static_cast", "reinterpret_cast", "const_cast",

    // atomic operations
    "atomic_cancel", "atomic_commit", "atomic_noexcept",

    // miscellaneous
    "sizeof", "typedef", "asm", "noexcept", "this", "reflexpr", "synchronized",

    // alternative tokens
    "and", "or", "not",
    "and_eq", "or_eq", "not_eq",
    "bitand", "bitor",
    "xor", "xor_eq",

    // additional
    "import", "module", "concepts", "final", "override"};

#define keywords_count (sizeof(keywords) / sizeof(keywords[0]))

const char *token_kind_name(Token_Kind kind)
{
    switch (kind)
    {
    case TOKEN_END:
        return "end of content";
    case TOKEN_INVALID:
        return "invalid token";
    case TOKEN_PREPROC:
        return "preprocessor directive";
    case TOKEN_SYMBOL:
        return "symbol";
    case TOKEN_OPEN_PAREN:
        return "open paren";
    case TOKEN_CLOSE_PAREN:
        return "close paren";
    case TOKEN_OPEN_CURLY:
        return "open curly";
    case TOKEN_CLOSE_CURLY:
        return "close curly";
    case TOKEN_SEMICOLON:
        return "semicolon";
    case TOKEN_KEYWORD:
        return "keyword";
    case TOKEN_OPERATOR:
        return "operator";
    default:
        UNREACHABLE("token_kind_name");
    }
    return NULL;
}

Lexer lexer_new(Free_Glyph_Atlas *atlas, const char *content, size_t content_len)
{
    Lexer lex = {0};
    lex.atlas = atlas;
    lex.content = content;
    lex.content_len = content_len;
    return lex;
}

bool lexer_starts_with(Lexer *lex, const char *prefix)
{
    size_t prefix_len = strlen(prefix);
    if (prefix_len == 0)
    {
        return true;
    }
    if (lex->cursor + prefix_len - 1 >= lex->content_len)
    {
        return false;
    }
    for (size_t i = 0; i < prefix_len; ++i)
    {
        if (prefix[i] != lex->content[lex->cursor + i])
        {
            return false;
        }
    }
    return true;
}

void lexer_chop_char(Lexer *lex, size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        // TODO: get rid of this assert by checking the length of the choped prefix upfront
        assert(lex->cursor < lex->content_len);
        char x = lex->content[lex->cursor];
        lex->cursor += 1;
        if (x == '\n')
        {
            lex->line += 1;
            lex->bol = lex->cursor;
            lex->x = 0;
        }
        else
        {
            if (lex->atlas)
            {
                size_t glyph_index = x;
                // TODO: support for glyphs outside of ASCII range
                if (glyph_index >= GLYPH_METRICS_CAPACITY)
                {
                    glyph_index = '?';
                }
                Glyph_Metric metric = lex->atlas->metrics[glyph_index];
                lex->x += metric.ax;
            }
        }
    }
}

void lexer_trim_left(Lexer *lex)
{
    while (lex->cursor < lex->content_len && isspace(lex->content[lex->cursor]))
    {
        lexer_chop_char(lex, 1);
    }
}

bool is_symbol_start(char x)
{
    return isalpha(x) || x == '_';
}

bool is_symbol(char x)
{
    return isalnum(x) || x == '_';
}

bool is_operator(char x)
{
    return x == '+' || x == '-' || x == '*' || x == '/' || x == '%' ||
           x == '<' || x == '>' || x == '=' || x == '!' || x == '&' ||
           x == '|' || x == '^' || x == '~' || x == '[' || x == ']' ||
           x == ',' || x == '.' || x == ':' || x == '?';
}

void handle_sequence(Lexer *lex)
{
    char ch = lex->content[lex->cursor];
    if (ch == '\\')
    {
        lexer_chop_char(lex, 1);
        if (lex->cursor < lex->content_len)
        {
            char next = lex->content[lex->cursor];
            if (next == 'n' || next == 't' || next == '\\' || next == '"' || next == '\'')
            {
                lexer_chop_char(lex, 1);
            }
        }
    }
    else
    {
        lexer_chop_char(lex, 1);
    }
}

Token lexer_next(Lexer *lex)
{
    lexer_trim_left(lex);

    Token token = {
        .text = &lex->content[lex->cursor],
    };

    token.position.x = lex->x;
    token.position.y = -(float)lex->line * FREE_GLYPH_FONT_SIZE * LINE_SPACING_FACTOR;

    if (lex->cursor >= lex->content_len)
        return token;

    if (isdigit(lex->content[lex->cursor]))
    {
        token.kind = TOKEN_NUMBER;
        // scan digits
        while (lex->cursor < lex->content_len && isdigit(lex->content[lex->cursor]))
        {
            lexer_chop_char(lex, 1);
        }
        token.text_len = (size_t)(&lex->content[lex->cursor] - token.text); // <--- added
        return token;
    }

    if (lex->content[lex->cursor] == '"')
    {
        token.kind = TOKEN_STRING;
        lexer_chop_char(lex, 1);
        while (lex->cursor < lex->content_len && lex->content[lex->cursor] != '"' && lex->content[lex->cursor] != '\n')
        {
            if (lex->content[lex->cursor] == '\\')
            {
                handle_sequence(lex);
            }
            else
            {
                lexer_chop_char(lex, 1);
            }
        }
        if (lex->cursor < lex->content_len && lex->content[lex->cursor] == '"')
        {
            lexer_chop_char(lex, 1);
        }
        token.text_len = &lex->content[lex->cursor] - token.text;
        return token;
    }

    if (lex->content[lex->cursor] == '#')
    {
        token.kind = TOKEN_PREPROC;
        while (lex->cursor < lex->content_len && lex->content[lex->cursor] != ' ')
        {
            lexer_chop_char(lex, 1);
        }
        if (lex->cursor < lex->content_len)
        {
            lexer_chop_char(lex, 1);
        }
        token.text_len = &lex->content[lex->cursor] - token.text;
        return token;
    }

    if (lexer_starts_with(lex, "//"))
    {
        token.kind = TOKEN_COMMENT;
        while (lex->cursor < lex->content_len && lex->content[lex->cursor] != '\n')
        {
            lexer_chop_char(lex, 1);
        }
        if (lex->cursor < lex->content_len)
        {
            lexer_chop_char(lex, 1);
        }
        token.text_len = &lex->content[lex->cursor] - token.text;
        return token;
    }

    if (is_operator(lex->content[lex->cursor]))
    {
        token.kind = TOKEN_OPERATOR;
        token.text_len = 1;
        lexer_chop_char(lex, 1);
        return token;
    }

    for (size_t i = 0; i < literal_tokens_count; ++i)
    {
        if (lexer_starts_with(lex, literal_tokens[i].text))
        {
            size_t text_len = strlen(literal_tokens[i].text);
            token.kind = literal_tokens[i].kind;
            token.text_len = text_len;
            lexer_chop_char(lex, text_len);
            return token;
        }
    }

    if (is_symbol_start(lex->content[lex->cursor]))
    {
        token.kind = TOKEN_SYMBOL;
        while (lex->cursor < lex->content_len && is_symbol(lex->content[lex->cursor]))
        {
            lexer_chop_char(lex, 1);
        }
        token.text_len = (size_t)(&lex->content[lex->cursor] - token.text); // <--- replaced uninitialized increment

        for (size_t i = 0; i < keywords_count; ++i)
        {
            size_t keyword_len = strlen(keywords[i]);
            if (keyword_len == token.text_len && memcmp(keywords[i], token.text, keyword_len) == 0)
            {
                token.kind = TOKEN_KEYWORD;
                break;
            }
        }

        return token;
    }

    lexer_chop_char(lex, 1);
    token.kind = TOKEN_INVALID;
    token.text_len = 1;
    return token;
}
