/******************************************************************************
  @file   c_unescape.c
  @brief

  DESCRIPTION: utility to unescape the c-style string with escape characters.
 
  https://en.cppreference.com/w/cpp/language/escape
 
  Parser works by marking on each character a state suitable for next character.
  This doesn't support the unicode escape sequence added in c++11.
  When it encounters an arbitrary termination (or finalization with out completing
  the escape sequence) the prior characters are unescaped and written to output
  buffer.
****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define C_ESCAPE_CHAR '\\'

/**
 * pass the corresponding character insert
 */
static inline uint8_t unescape_char(uint8_t c)
{
    switch (c) {
    case '\'': return '\'';
    case '\"': return '\"';
    case '\?': return '\?';
    case '\\': return '\\';
    case 'a': return '\a';
    case 'b': return '\b';
    case 'f': return '\f';
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'v': return '\v';
    default: return c;
    }
}

static inline bool is_octal_digit(uint8_t c, uint8_t* val)
{
    if ('0' <= c && c < '8') {
        *val = c - '0';
        return true;
    }
    return false;
}

static inline bool is_hex_digit(uint8_t c, uint8_t* val)
{
    if ('0' <= c && c <= '9') {
        *val = c - '0';
        return true;
    }

    if ('A' <= c && c <= 'F') {
        *val = c - 'A' + 10;
        return true;
    }

    if ('a' <= c && c <= 'f') {
        *val = c - 'a' + 10;
        return true;
    }

    return false;
}

static inline uint8_t octal_value_1(uint8_t v)
{
    return v;
}

static inline uint8_t octal_value_2(uint8_t v1, uint8_t v2)
{
    return (v1 << 3) | (v2);
}

static inline uint8_t octal_value_3(uint8_t v1, uint8_t v2, uint8_t v3)
{
    return (v1 << 6) | (v2 << 3) | (v3);   /* max \377 */
}

static inline uint8_t hex_value_1(uint8_t v)
{
    return v;
}

static inline uint8_t hex_value_2(uint8_t v1, uint8_t v2)
{
    return (v1 << 4) | (v2);
}

enum unescape_parser_s {
    UNESCAPE_PARSER_S_NONE,      /* nothing to escape */
    UNESCAPE_PARSER_S_BACKSLASH, /* \ was encountered */
    UNESCAPE_PARSER_S_HEX,       /* \x was encountered */
    UNESCAPE_PARSER_S_HEX1,      /* \xH was encountered, H is a valid hex digit */
    UNESCAPE_PARSER_S_OCTAL1,    /* \o was encountered, o is a valid octal digit  */
    UNESCAPE_PARSER_S_OCTAL2,    /* \oo was encountered each o is a valid octal digit */
};

struct c_unescape_parser {
    uint8_t v1;   /* v1 and v2 are used to cache the values seen in the midst of escape sequence */
    uint8_t v2;
    enum unescape_parser_s st;
    size_t required;  /* total output buffer required */
    uint8_t* dest;
    size_t dest_len;
};

void c_unescape_init(struct c_unescape_parser* parser, uint8_t* dest,
                     size_t dest_len)
{
    parser->st = UNESCAPE_PARSER_S_NONE;
    parser->required = 0;
    parser->dest = dest;
    parser->dest_len = dest_len;
}

static inline void c_unescape_append(struct c_unescape_parser* parser, uint8_t c)
{
    if (parser->dest_len) { *parser->dest++ = c; parser->dest_len--; }
    parser->required++;
}

/* one character input */
static void c_unescape_process_one(struct c_unescape_parser* parser, uint8_t c)
{
    switch (parser->st) {
    case UNESCAPE_PARSER_S_BACKSLASH:
        if (is_octal_digit(c, &parser->v1)) {
            parser->st = UNESCAPE_PARSER_S_OCTAL1;
        }
        else if ('x' == c) {
            parser->st = UNESCAPE_PARSER_S_HEX;
        }
        else {
            parser->st = UNESCAPE_PARSER_S_NONE;
            c_unescape_append(parser, unescape_char(c));
        }
        break;
    case UNESCAPE_PARSER_S_HEX:
        if (is_hex_digit(c, &parser->v1)) {
            parser->st = UNESCAPE_PARSER_S_HEX1;
        }
        else {
            c_unescape_append(parser, unescape_char('x'));
            if (c == C_ESCAPE_CHAR) {
                parser->st = UNESCAPE_PARSER_S_BACKSLASH;
            }
            else {
                parser->st = UNESCAPE_PARSER_S_NONE;
                c_unescape_append(parser, c);
            }
        }
        break;
    case UNESCAPE_PARSER_S_HEX1:
        if (is_hex_digit(c, &c)) {
            parser->st = UNESCAPE_PARSER_S_NONE;
            c_unescape_append(parser, hex_value_2(parser->v1, c));
        }
        else {
            c_unescape_append(parser, hex_value_1(parser->v1));
            if (c == C_ESCAPE_CHAR) {
                parser->st = UNESCAPE_PARSER_S_BACKSLASH;
            }
            else {
                parser->st = UNESCAPE_PARSER_S_NONE;
                c_unescape_append(parser, c);
            }
        }
        break;
    case UNESCAPE_PARSER_S_OCTAL1:
        if (is_octal_digit(c, &parser->v2)) {
            parser->st = UNESCAPE_PARSER_S_OCTAL2;
        }
        else {
            c_unescape_append(parser, octal_value_1(parser->v1));
            if (c == C_ESCAPE_CHAR) {
                parser->st = UNESCAPE_PARSER_S_BACKSLASH;
            }
            else {
                parser->st = UNESCAPE_PARSER_S_NONE;
                c_unescape_append(parser, c);
            }
        }
        break;
    case UNESCAPE_PARSER_S_OCTAL2:
        if (is_octal_digit(c, &c)) {
            parser->st = UNESCAPE_PARSER_S_NONE;
            c_unescape_append(parser, octal_value_3(parser->v1, parser->v2, c));
        }
        else {
            c_unescape_append(parser, octal_value_2(parser->v1, parser->v2));
            if (c == C_ESCAPE_CHAR) {
                parser->st = UNESCAPE_PARSER_S_BACKSLASH;
            }
            else {
                parser->st = UNESCAPE_PARSER_S_NONE;
                c_unescape_append(parser, c);
            }
        }
        break;
    case UNESCAPE_PARSER_S_NONE:
    default:
        if (c == C_ESCAPE_CHAR) {
            parser->st = UNESCAPE_PARSER_S_BACKSLASH;
        }
        else {
            c_unescape_append(parser, c);
        }
        break;
    }
}

static void c_unescape_finalize(struct c_unescape_parser* parser)
{
    switch (parser->st) {
    case UNESCAPE_PARSER_S_BACKSLASH:
        c_unescape_append(parser, unescape_char('\\'));
        break;
    case UNESCAPE_PARSER_S_HEX:
        c_unescape_append(parser, unescape_char('x'));
        break;
    case UNESCAPE_PARSER_S_HEX1:
        c_unescape_append(parser, hex_value_1(parser->v1));
        break;
    case UNESCAPE_PARSER_S_OCTAL1:
        c_unescape_append(parser, octal_value_1(parser->v1));
        break;
    case UNESCAPE_PARSER_S_OCTAL2:
        c_unescape_append(parser, octal_value_2(parser->v1, parser->v2));
        break;
    case UNESCAPE_PARSER_S_NONE:
    default:
        ;
    }
}

int c_unescape(const uint8_t* src, size_t src_len, uint8_t* dest, size_t dest_len,
               size_t* dest_len_required)
{
    uint8_t c;
    struct c_unescape_parser parser;

    c_unescape_init(&parser, dest, dest_len);

    while (src_len) {
        c = *src++; src_len--;
        c_unescape_process_one(&parser, c);
    }

    c_unescape_finalize(&parser);

    if (0 != dest_len_required) {
        *dest_len_required = parser.required;
    }

    return 0;
}

// test code
// 
#include <stdio.h>
#include <string.h>

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))
int main(void)
{
    struct {
        const uint8_t* data;
        const uint8_t* result;
        int expected_length;
    } t[] = {
        {"ab\\xFF\\03\\7\\377\\t\\?\\'\\\\yz", "ab\xFF\03\7\377\t\?\'\\yz", 12},
        {"ab\\xFF\\03\\7\\377\\t\\?\\'\\\\\x0z", "ab\xFF\03\7\377\t\?\'\\\x0z", 12}
    };
    
    const char data1[] = "ab\\xFF\\03\\7\\377\\t\\?\\'\\\\yz";
    const char res1[] = "ab\xFF\03\7\377\t\?\'\\yz";
    const char data2[] = "ab\\xFF\\03\\7\\377\\t\\?\\'\\\\\x0z";
    const char res2[] = "ab\xFF\03\7\377\t\?\'\\\x0z";
    uint8_t buffer1[20];
    uint8_t buffer2[20];
    size_t req1, req2;

    c_unescape((const uint8_t*)data1, strlen(data1), buffer1, sizeof(buffer1), &req1);
    if (req1 != 12 || 0 != memcmp(buffer1, res1, req1)) {
        return 1;
    }

    c_unescape((const uint8_t*)data2, strlen(data2), buffer2, sizeof(buffer2), &req2);
    c_unescape(t[1].data, strlen(t[1].data), buffer2, sizeof(buffer2), &req2);
    if (req2 != 12 || 0 != memcmp(buffer2, res2, req2)) {
        return 1;
    }
    printf("PASS num bytes : %lu\n", req2);

    for (int i = 0; i < NELEMS(t); i++) {
        size_t req;
        uint8_t buffer[20];

        c_unescape(t[i].data, strlen(t[i].data), buffer, sizeof(buffer), &req);

        printf("%s\n", (req == t[i].expected_length
               && 0 == memcmp(buffer, t[i].result, req)) ? "PASS" : "FAIL");
    }
    return 0;
}