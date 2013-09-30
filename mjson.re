#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>
#include <memory.h>

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

#define BJSON_ID_NULL            0
#define BJSON_ID_FALSE           1
#define BJSON_ID_EMPTY_STRING    2
#define BJSON_ID_TRUE            3

#define BJSON_ID_UINT8           4
#define BJSON_ID_UINT16          5
#define BJSON_ID_UINT32          6
#define BJSON_ID_UINT64          7

#define BJSON_ID_SINT8           8
#define BJSON_ID_SINT16          9
#define BJSON_ID_SINT32         10
#define BJSON_ID_SINT64         11

#define BJSON_ID_FLOAT32        12
#define BJSON_ID_FLOAT64        13

#define BJSON_ID_UTF8_STRING8   16
#define BJSON_ID_UTF8_STRING16  17
#define BJSON_ID_UTF8_STRING32  18
#define BJSON_ID_UTF8_STRING64  19

#define BJSON_ID_BINARY8        20
#define BJSON_ID_BINARY16       21
#define BJSON_ID_BINARY32       22
#define BJSON_ID_BINARY64       23

#define BJSON_ID_ARRAY8         32
#define BJSON_ID_ARRAY16        33
#define BJSON_ID_ARRAY32        34
#define BJSON_ID_ARRAY64        35

#define BJSON_ID_DICT8          36
#define BJSON_ID_DICT16         37
#define BJSON_ID_DICT32         38
#define BJSON_ID_DICT64         39

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

static int parse_number(mjson_parser_t *context)
{
    int              num_parsed;
    uint8_t          bjson_id;
    const char*      format;
    bjson_entry32_t* bdata;

    switch(context->token)
    {
        case TOK_OCT_NUMBER:
            bjson_id = BJSON_ID_SINT32;
            format   = "%o";
            break;
        case TOK_HEX_NUMBER:
            bjson_id = BJSON_ID_SINT32;
            format   = "%x";
            break;
        case TOK_DEC_NUMBER:
            bjson_id = BJSON_ID_SINT32;
            format   = "%d";
            break;
        case TOK_FLOAT_NUMBER:
            bjson_id = BJSON_ID_FLOAT32;
            format   = "%f";
            break;
        default:
            assert(!"unknown token");
    }

    if (context->bjson_limit - context->bjson < sizeof(bjson_entry32_t)) return 0;
    
    bdata           = (bjson_entry32_t*)context->bjson;
    bdata->id       = bjson_id;
    context->bjson += sizeof(bjson_entry32_t);

    num_parsed = sscanf(context->start, format, &bdata->val_u32);
    assert(num_parsed == 1);

    mjson_next_token(context);
    return 1;
}

static int parse_identifier(mjson_parser_t *context)
{
    bjson_entry32_t* bdata;
    size_t           str_len;

    str_len = context->next - context->start;

    if (context->bjson_limit - context->bjson < sizeof(uint8_t) + str_len) return 0;
    
    bdata          = (bjson_entry32_t*)context->bjson;
    bdata->id      = BJSON_ID_UTF8_STRING32;
    bdata->val_u32 = str_len;

    context->bjson += sizeof(bjson_entry32_t);

    memcpy(context->bjson, context->start, str_len);
    context->bjson += str_len;

    mjson_next_token(context);
    return 1;
}

static int parse_string(mjson_parser_t *context)
{
    bjson_entry32_t* bdata;
    size_t           str_len;

    str_len = context->next - context->start - 2;

    if (context->bjson_limit - context->bjson < sizeof(uint8_t) + str_len) return 0;
    
    bdata          = (bjson_entry32_t*)context->bjson;
    bdata->id      = BJSON_ID_UTF8_STRING32;
    bdata->val_u32 = str_len;

    context->bjson += sizeof(bjson_entry32_t);

    memcpy(context->bjson, context->start + 1, str_len);
    context->bjson += str_len;

    mjson_next_token(context);
    return 1;
}

static int parse_escaped_string(mjson_parser_t *context)
{
    assert(!"implemented");
    mjson_next_token(context);
    return 1;
}

static int parse_value(mjson_parser_t *context)
{
    assert(context);
 
    switch (context->token)
    {
        case TOK_NULL:
            if (context->bjson_limit - context->bjson < sizeof(uint8_t)) return 0;
            *(uint8_t*)context->bjson = BJSON_ID_NULL;
            context->bjson += sizeof(uint8_t);
            mjson_next_token(context);
            break;
        case TOK_FALSE:
            if (context->bjson_limit - context->bjson < sizeof(uint8_t)) return 0;
            *(uint8_t*)context->bjson = BJSON_ID_FALSE;
            context->bjson += sizeof(uint8_t);
            mjson_next_token(context);
            break;
        case TOK_TRUE:
            if (context->bjson_limit - context->bjson < sizeof(uint8_t)) return 0;
            context->bjson += sizeof(uint8_t);
            *(uint8_t*)context->bjson = BJSON_ID_TRUE;
            mjson_next_token(context);
            break;
        case TOK_OCT_NUMBER:
        case TOK_HEX_NUMBER:
        case TOK_DEC_NUMBER:
        case TOK_FLOAT_NUMBER:
            if (!parse_number(context))
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
    bjson_entry32_t* array;
    uint8_t*         data_start;
    int              expect_separator;

    assert(context);

    if (context->bjson_limit - context->bjson < sizeof(bjson_entry32_t)) return 0;

    array     = (bjson_entry32_t*)context->bjson;
    array->id = BJSON_ID_ARRAY32;

    context->bjson += sizeof(bjson_entry32_t);
    data_start      = context->bjson;

    expect_separator = FALSE;

    while (context->token != TOK_RIGHT_BRACKET)
    {
        if (expect_separator && context->token == TOK_COMMA)
            mjson_next_token(context);
        else
            expect_separator = TRUE;

        if (!parse_value(context))
            return 0;
    }

    array->val_u32 = context->bjson - data_start;

    mjson_next_token(context);

    return 1;
}

static int parse_key_value_pair(mjson_parser_t* context, int stop_token)
{
    bjson_entry32_t* dictionary;
    uint8_t*         data_start;
    int              expect_separator;
 
    assert(context);

    if (context->bjson_limit - context->bjson < sizeof(bjson_entry32_t)) return 0;

    dictionary     = (bjson_entry32_t*)context->bjson;
    dictionary->id = BJSON_ID_DICT32;

    context->bjson += sizeof(bjson_entry32_t);
    data_start      = context->bjson;
    
    expect_separator = FALSE;
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

    dictionary->val_u32 = context->bjson - data_start;
    
    mjson_next_token(context);

    return 1;
}
