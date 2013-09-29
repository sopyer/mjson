#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>

#include "mjson.h"

/* mjson: - no {} needed around the whole file
- "=" is allowed instead of ":"
- quotes around the key are optional
- commas after values are optional 
- and c-style comments allowed */

#define TOK_NONE                   0
#define TOK_IDENTIFIER             1
#define TOK_NOESC_STRING           2
#define TOK_STRING                 3
#define TOK_OCT_NUMBER             4
#define TOK_HEX_NUMBER             5
#define TOK_DEC_NUMBER             6
#define TOK_FLOAT_NUMBER           7
#define TOK_COMMA                  8
#define TOK_COLON                  9
#define TOK_EQUAL                 10
#define TOK_LEFT_BRACKET          11
#define TOK_RIGHT_BRACKET         12
#define TOK_LEFT_CURVE_BRACKET    13
#define TOK_RIGHT_CURVE_BRACKET   14
#define TOK_FALSE                 15
#define TOK_TRUE                  16
#define TOK_NULL                  17
#define TOK_WHITESPACE            18
#define TOK_INVALID               19

#define TRUE  1
#define FALSE 0
#define return_val_if_fail(cond, val) if (!(cond)) return (val)
#define return_if_fail(cond) if (!(cond)) return
#define INT64_T_FORMAT "lld"
#define MAX_UTF8_CHAR_LEN 6

typedef unsigned int  uint;
typedef unsigned char uchar;

static void unicode_cp_to_utf8(uint32_t uni_cp, char* utf8char/*[6]*/, size_t* charlen)
{
    uint32_t first, i;
    
    if (uni_cp < 0x80)
    {
        first   = 0;
        *charlen = 1;
    }
    else if (uni_cp < 0x800)
    {
        first   = 0xc0;
        *charlen = 2;
    }
    else if (uni_cp < 0x10000)
    {
        first   = 0xe0;
        *charlen = 3;
    }
    else if (uni_cp < 0x200000)
    {
        first   = 0xf0;
        *charlen = 4;
    }
    else if (uni_cp < 0x4000000)
    {
        first   = 0xf8;
        *charlen = 5;
    }
    else
    {
        first   = 0xfc;
        *charlen = 6;
    }

    for (i = *charlen - 1; i > 0; --i)
    {
        utf8char[i] = (uni_cp & 0x3f) | 0x80;
        uni_cp >>= 6;
    }
    utf8char[0] = uni_cp | first;
}

static void mjson_next_token(mjson_parser_t* context)
{
#define YYREADINPUT(c) (c>=e?0:*c)
#define YYCTYPE        char
#define YYCURSOR       c
#define YYMARKER       m

/*!re2c
    re2c:yyfill:enable      = 0;
    re2c:indent:top         = 2;
    re2c:indent:string      = "    ";

    WS = [ \t\n\r]+;

    S = [+-];
    O = [0-7];
    D = [0-9];
    H = [a-fA-F0-9];
    L = [a-zA-Z_];
    E = [Ee] [+-]? D+;

    HEX_NUMBER = ("0" [xX] H+);
    OCT_NUMBER = ("0" O+);
    DEC_NUMBER = (S? ("0"|([1-9]D*)));

    FLOAT_NUMBER = (S? D+ E) | (S? D* "." D+ E?) | (S? D+ "." D* E?);

    CHAR               = [^\\"\000];
    CTL                = "\\" ["\\/bfnrt];
    UNICODE            = "\\u" H{4};
    STRING             = "\"" (CHAR|CTL|UNICODE)* "\"";
    NOESC_STRING       = "\"" (CHAR)* "\"";
    IDENTIFIER         = L (L|D)*;
    SINGLELINE_COMMENT = "//" [^\n\000]* "\n";
    MULTILINE_COMMENT  = "\/*" [^*\000]* [*]+ ( [^\/\000] [^*\000]* [*]+ )* "\/";
*/

    const char* c = context->next;
    const char* e = context->end;
    const char* m = NULL;
    const char* s;
    int token = TOK_NONE;

    assert(context);
    return_if_fail(context->next != NULL);

    while (TRUE)
    {
        s = c;

/*!re2c
            WS {
                continue; 
            }
            
            SINGLELINE_COMMENT {
                continue; 
            }
            
            MULTILINE_COMMENT {
                continue; 
            }
            
            "{" {
                token = TOK_LEFT_CURVE_BRACKET;
                goto done;
            }
    
            "}" {
                token = TOK_RIGHT_CURVE_BRACKET;
                goto done;
            }
    
            "[" {
                token = TOK_LEFT_BRACKET;
                goto done;
            }
    
            "]" {
                token = TOK_RIGHT_BRACKET;
                goto done;
            }
    
            ":" {
                token = TOK_COLON;
                goto done;
            }
    
            "=" {
                token = TOK_EQUAL;
                goto done;
            }

            "," {
                token = TOK_COMMA;
                goto done;
            }
    
            OCT_NUMBER {
                token = TOK_OCT_NUMBER;
                goto done;
            }
    
            HEX_NUMBER {
                token = TOK_HEX_NUMBER;
                goto done;
            }
    
            DEC_NUMBER {
                token = TOK_DEC_NUMBER;
                goto done;
            }
    
            FLOAT_NUMBER {
                token = TOK_FLOAT_NUMBER;
                goto done;
            }
    
            "true" {
                token = TOK_TRUE;
                goto done;
            }
    
            "false" {
                token = TOK_FALSE;
                goto done;
            }
    
            "null" {
                token = TOK_NULL;
                goto done;
            }
    
            IDENTIFIER {
                token = TOK_IDENTIFIER;
                goto done;
            }

            NOESC_STRING {
                token = TOK_NOESC_STRING;
                goto done;
            }
    
            STRING {
                token = TOK_STRING;
                goto done;
            }
    
            [\000] { 
                context->token = TOK_NONE;
                return;
            }
    
            (L|D)+ {
                context->token = TOK_INVALID;
                return;
            }

            . | "\n" {
                context->token = TOK_INVALID;
                return;
            }
*/
    }

done:
    context->token = token;
    context->start = s;
    context->next  = c;

#undef YYREADINPUT
#undef YYCTYPE           
#undef YYCURSOR          
#undef YYMARKER          
}

static int parse_value_list (mjson_parser_t *context);
static int parse_key_value_pair(mjson_parser_t *context, int stop_token);

int mjson_parse(const char *json_data, size_t json_data_size, void* bjson_data, size_t bjson_data_size)
{
    mjson_parser_t c = {
        TOK_NONE, 0,
        json_data, json_data + json_data_size,
        (uint8_t*)bjson_data, (uint8_t*)bjson_data + bjson_data_size
    };
    int stop_token = TOK_NONE;

    mjson_next_token(&c);
    
    if (c.token == TOK_LEFT_BRACKET)
    {
        mjson_next_token(&c);
        if (!parse_value_list(&c))
            return 0;
    }
    else
    {
        if (c.token == TOK_LEFT_CURVE_BRACKET)
        {
            stop_token = TOK_RIGHT_CURVE_BRACKET;
            mjson_next_token(&c);
        }

        if (!parse_key_value_pair(&c, stop_token))
            return 0;
    }

    if (c.token != TOK_NONE)
        return 0;

    return 1;
}

static int parse_oct_number(mjson_parser_t *context)
{
    mjson_next_token(context);
    return 1;
}

static int parse_hex_number(mjson_parser_t *context)
{
    mjson_next_token(context);
    return 1;
}

static int parse_dec_number(mjson_parser_t *context)
{
    mjson_next_token(context);
    return 1;
}

static int parse_float_number(mjson_parser_t *context)
{
    mjson_next_token(context);
    return 1;
}

static int parse_identifier(mjson_parser_t *context)
{
    mjson_next_token(context);
    return 1;
}

static int parse_string(mjson_parser_t *context)
{
    mjson_next_token(context);
    return 1;
}

static int parse_escaped_string(mjson_parser_t *context)
{
    mjson_next_token(context);
    return 1;
}

static int parse_value(mjson_parser_t *context)
{
    const char* value = context->start;
 
    assert(context);
 
    switch (context->token)
    {
        case TOK_NULL:
            mjson_next_token(context);
            break;
        case TOK_FALSE:
            mjson_next_token(context);
            break;
        case TOK_TRUE:
            mjson_next_token(context);
            break;
        case TOK_OCT_NUMBER:
            if (!parse_oct_number(context))
                return 0;
            break;
        case TOK_HEX_NUMBER:
            if (!parse_hex_number(context))
                return 0;
            break;
        case TOK_DEC_NUMBER:
            if (!parse_dec_number(context))
                return 0;
            break;
        case TOK_FLOAT_NUMBER:
            if (!parse_float_number(context))
                return 0;
            break;
        case TOK_NOESC_STRING:
            if (!parse_string(context))
                return 0;
            break;
        case TOK_STRING:
            if (!parse_escaped_string(context))
                return 0;
            break;
        case TOK_LEFT_CURVE_BRACKET:
            mjson_next_token(context);
            if (!parse_key_value_pair(context, TOK_RIGHT_CURVE_BRACKET))
                return 0;
            break;
        case TOK_LEFT_BRACKET:
            mjson_next_token(context);
            if (!parse_value_list(context))
                return 0;
            break;
        default:
            return 0;
    }

    return 1;
}

static int parse_value_list(mjson_parser_t *context)
{
    mjson_data32_t* array            = (mjson_data32_t*)context->bjson;
    uint8_t*        data_start       = (uint8_t*)(array+1);
    int             expect_separator = FALSE;

    assert(context);
    
    context->bjson = data_start;
    array->id      = BJSON_ID_ARRAY32
    
    while (context->token != TOK_RIGHT_BRACKET)
    {
        if (expect_separator && context->token == TOK_COMMA)
            mjson_next_token(context);
        else
            expect_separator = TRUE;

        if (!parse_value(context))
            return 0;
    }

    array->data_size = context->bjson - data_start;

    mjson_next_token(context);

    return 1;
}

static int parse_key_value_pair(mjson_parser_t* context, int stop_token)
{
    mjson_data32_t* dictionary       = (mjson_data32_t*)context->bjson;
    uint8_t*        data_start       = (uint8_t*)(dictionary+1);
    int             expect_separator = FALSE;
 
    assert(context);

    context->bjson = data_start;
    dictionary->id = BJSON_ID_DICT32
    
    while (context->token != stop_token)
    {
        if (expect_separator && context->token == TOK_COMMA)
            mjson_next_token(context);
        else
            expect_separator = TRUE;

        switch (context->token)
        {
            case TOK_IDENTIFIER:
                if (!parse_identifier(context))
                    return 0;
                break;        
            case TOK_NOESC_STRING:
                if (!parse_string(context))
                    return 0;
                break;        
            default:
                return 0;
        }

        if (context->token != TOK_COLON && context->token != TOK_EQUAL)
            return 0;

        mjson_next_token(context);

        if (!parse_value(context))
            return 0;
    }

    dictionary->data_size = context->bjson - data_start;
    
    mjson_next_token(context);

    return 1;
}
