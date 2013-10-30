#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>

#include "mjson.h"

enum mjson_token_t
{
    TOK_NONE                =  0,
    TOK_IDENTIFIER          =  1,
    TOK_NOESC_STRING        =  2,
    TOK_STRING              =  3,
    TOK_OCT_NUMBER          =  4,
    TOK_HEX_NUMBER          =  5,
    TOK_DEC_NUMBER          =  6,
    TOK_FLOAT_NUMBER        =  7,
    TOK_COMMA               =  8,
    TOK_COLON               =  9,
    TOK_EQUAL               = 10,
    TOK_LEFT_BRACKET        = 11,
    TOK_RIGHT_BRACKET       = 12,
    TOK_LEFT_CURLY_BRACKET  = 13,
    TOK_RIGHT_CURLY_BRACKET = 14,
    TOK_FALSE               = 15,
    TOK_TRUE                = 16,
    TOK_NULL                = 17,
    TOK_WHITESPACE          = 18,
    TOK_INVALID             = 19
};

struct _mjson_parser_t
{
    int token;
    const char* start;
    const char* next;
    const char* end;
    uint8_t* bjson;
    uint8_t* bjson_limit;
};

#ifdef _MSC_VER
#   pragma pack(push, 1)
#else
#   error "proper packing not implemented!!!"
#endif

struct _mjson_entry_t
{
    uint32_t  id;
    union
    {
        uint32_t val_u32;
        int32_t  val_s32;
        float    val_f32;
    };
};

#ifdef _MSC_VER
#   pragma pack(pop)
#else
#   error "proper packing not implemented!!!"
#endif

#define return_val_if_fail(cond, val) if (!(cond)) return (val)
#define return_if_fail(cond) if (!(cond)) return
#define INT64_T_FORMAT "lld"
#define MAX_UTF8_CHAR_LEN 6
#define TRUE  1
#define FALSE 0

typedef struct _mjson_parser_t  mjson_parser_t;
typedef struct _mjson_entry_t   mjson_entry_t;

static void* parsectx_allocate_output(mjson_parser_t* ctx, ptrdiff_t size);

static void parsectx_next_token    (mjson_parser_t* context);

static int parse_value_list    (mjson_parser_t *context);
static int parse_key_value_pair(mjson_parser_t *context, int stop_token);

static mjson_element_t next_element(mjson_element_t element);

int mjson_parse(const char *json_data, size_t json_data_size, void* storage_buf, size_t storage_buf_size, const mjson_entry_t** top_element)
{
    uint32_t* fourcc;
    mjson_parser_t c = {
        TOK_NONE, 0,
        json_data, json_data + json_data_size,
        (uint8_t*)storage_buf, (uint8_t*)storage_buf + storage_buf_size
    };
    int stop_token = TOK_NONE;

    *top_element = 0;

    fourcc = (uint32_t*)parsectx_allocate_output(&c, (ptrdiff_t)sizeof(uint32_t));

    if (!fourcc) return 0;

    *fourcc = '23JB';

    parsectx_next_token(&c);

    if (c.token == TOK_LEFT_BRACKET)
    {
        parsectx_next_token(&c);
        if (!parse_value_list(&c))
            return 0;
    }
    else
    {
        if (c.token == TOK_LEFT_CURLY_BRACKET)
        {
            stop_token = TOK_RIGHT_CURLY_BRACKET;
            parsectx_next_token(&c);
        }

        if (!parse_key_value_pair(&c, stop_token))
            return 0;
    }

    if (c.token != TOK_NONE)
        return 0;

    *top_element = (mjson_entry_t*)(fourcc + 1);

    return 1;
}

mjson_element_t mjson_get_top_element(void* storage_buf, size_t storage_buf_size)
{
    mjson_element_t top = (mjson_element_t)storage_buf;
    
    return_val_if_fail(top, NULL);
    return_val_if_fail(top->id == MJSON_ID_DICT32 || top->id == MJSON_ID_ARRAY32, NULL);
    return_val_if_fail(top->val_u32 <= storage_buf_size, NULL);
    
    return top;
}

mjson_element_t mjson_get_element_first(mjson_element_t array)
{
    return_val_if_fail(array, NULL);
    return_val_if_fail(array->id == MJSON_ID_ARRAY32, NULL);
    
    return array + 1;
}

mjson_element_t mjson_get_element_next(mjson_element_t array, mjson_element_t current_value)
{
    mjson_element_t next = NULL;

    return_val_if_fail(array, NULL);
    return_val_if_fail(current_value, NULL);
    return_val_if_fail(array->id == MJSON_ID_ARRAY32, NULL);
    return_val_if_fail((uint8_t*)array + array->val_u32 > (uint8_t*)current_value, NULL);
    
    next = next_element(current_value);
    
    return_val_if_fail((uint8_t*)array + array->val_u32 > (uint8_t*)next, NULL);
    
    return next;
}

mjson_element_t mjson_get_element(mjson_element_t array, int index)
{
    mjson_element_t result;
    
    result = mjson_get_element_first(array);
    while (result && index--)
        result = mjson_get_element_next(array, result);
    
    return result;
}

mjson_element_t mjson_get_member_first(mjson_element_t dictionary, mjson_element_t* value)
{
    return_val_if_fail(dictionary, NULL);
    return_val_if_fail(dictionary->id == MJSON_ID_DICT32, NULL);
    return_val_if_fail((dictionary+1)->id == MJSON_ID_UTF8_KEY32, NULL);
    
    *value = next_element(dictionary+1);
    
    return dictionary + 1;
}

mjson_element_t mjson_get_member_next(mjson_element_t dictionary, mjson_element_t current_key, mjson_element_t* next_value)
{
    mjson_element_t next_key = NULL;

    return_val_if_fail(dictionary, NULL);
    return_val_if_fail(dictionary->id == MJSON_ID_DICT32, NULL);
    return_val_if_fail(current_key, NULL);
    return_val_if_fail((uint8_t*)dictionary + dictionary->val_u32 > (uint8_t*)current_key, NULL);
    return_val_if_fail(current_key->id == MJSON_ID_UTF8_KEY32, NULL);
    
    next_key = next_element(current_key);
    next_key = next_element(next_key);
    
    return_val_if_fail(next_key, NULL);
    return_val_if_fail((uint8_t*)dictionary + dictionary->val_u32 > (uint8_t*)next_key, NULL);
    return_val_if_fail(next_key->id == MJSON_ID_UTF8_KEY32, NULL);

    *next_value = next_element(next_key);
   
    return next_key;    
}

mjson_element_t mjson_get_member(mjson_element_t dictionary, const char* name)
{
    mjson_element_t key, result;
    
    key = mjson_get_member_first(dictionary, &result);
    while (key && strncmp(name, (char*)(key+1), key->val_u32) != 0)
        result = mjson_get_member_next(dictionary, key, &result);
    
    return result;
}

int mjson_get_type(mjson_element_t element)
{
    return_val_if_fail(element, MJSON_ID_NULL);
    
    return element->id;
}

const char* mjson_get_string(mjson_element_t element, const char* fallback)
{
    return_val_if_fail(element, fallback);
    return_val_if_fail(element->id == MJSON_ID_UTF8_STRING32, fallback);
    
    return (const char*)(element+1);
}

int32_t mjson_get_int(mjson_element_t element, int32_t fallback)
{
    return_val_if_fail(element, fallback);
    return_val_if_fail(element->id == MJSON_ID_SINT32, fallback);
    
    return element->val_s32;
}

float mjson_get_float(mjson_element_t element, float fallback)
{
    return_val_if_fail(element, fallback);
    return_val_if_fail(element->id == MJSON_ID_FLOAT32, fallback);
    
    return element->val_f32;
}

int mjson_get_bool(mjson_element_t element, int fallback)
{
    return_val_if_fail(element, fallback);
    return_val_if_fail(element->id == MJSON_ID_TRUE || element->id == MJSON_ID_FALSE, fallback);
    
    return element->id == MJSON_ID_TRUE;
}

int mjson_is_null(mjson_element_t element)
{
    return_val_if_fail(element, TRUE);

    return element->id == MJSON_ID_NULL;
}

/////////////////////////////////////////////////////////////////////////////
// API helpers
/////////////////////////////////////////////////////////////////////////////

static size_t element_size(mjson_element_t element)
{
    return_val_if_fail(element, 0);

    switch(element->id)
    {
        case MJSON_ID_NULL:
        case MJSON_ID_FALSE:
        case MJSON_ID_EMPTY_STRING:
        case MJSON_ID_TRUE:
            return sizeof(uint32_t);

        case MJSON_ID_UINT32:
        case MJSON_ID_SINT32:
        case MJSON_ID_FLOAT32:
            return sizeof(mjson_entry_t);

        case MJSON_ID_UTF8_KEY32:
        case MJSON_ID_UTF8_STRING32:
        case MJSON_ID_BINARY32:
        case MJSON_ID_ARRAY32:
        case MJSON_ID_DICT32:
            return sizeof(mjson_entry_t) + ((element->val_u32 + 3) & (~3));
    };

    return 0;
}

static mjson_element_t next_element(mjson_element_t element)
{
    size_t size;
    
    return_val_if_fail(element, 0);

    size = element_size(element);
    assert(size>0);
    
    return (mjson_element_t)((uint8_t*)element + size);
}

static void* parsectx_reserve_output(mjson_parser_t* ctx, ptrdiff_t size)
{
    return (ctx->bjson_limit - ctx->bjson < size) ? 0 : ctx->bjson;
}

static void parsectx_advance_output(mjson_parser_t* ctx, ptrdiff_t size)
{
    ctx->bjson += size;
}

static void* parsectx_allocate_output(mjson_parser_t* ctx, ptrdiff_t size)
{
    void* ptr;

    if (ctx->bjson_limit - ctx->bjson < size)
        return 0;

    ptr = ctx->bjson;
    ctx->bjson += size;

    return ptr;
}

//TODO: what about 64 bit code????
static void parsectx_align4_output(mjson_parser_t* ctx)
{
    ctx->bjson = (uint8_t*)(((ptrdiff_t)ctx->bjson + 3) & (~3));
}

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

/////////////////////////////////////////////////////////////////////////////
// Lexer+Parser code
/////////////////////////////////////////////////////////////////////////////

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

static void parsectx_next_token(mjson_parser_t* context)
{
#define YYREADINPUT(c) (c>=e?0:*c)
#define YYCTYPE        char
#define YYCURSOR       c
#define YYMARKER       m

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
                token = TOK_LEFT_CURLY_BRACKET;
                goto done;
            }
    
            "}" {
                token = TOK_RIGHT_CURLY_BRACKET;
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

static int parse_number(mjson_parser_t *context)
{
    int            num_parsed;
    uint8_t        bjson_id;
    const char*    format;
    mjson_entry_t* bdata;

    switch(context->token)
    {
        case TOK_OCT_NUMBER:
            bjson_id = MJSON_ID_SINT32;
            format   = "%o";
            break;
        case TOK_HEX_NUMBER:
            bjson_id = MJSON_ID_SINT32;
            format   = "%x";
            break;
        case TOK_DEC_NUMBER:
            bjson_id = MJSON_ID_SINT32;
            format   = "%d";
            break;
        case TOK_FLOAT_NUMBER:
            bjson_id = MJSON_ID_FLOAT32;
            format   = "%f";
            break;
        default:
            assert(!"unknown token");
    }

    bdata = (mjson_entry_t*)parsectx_allocate_output(context, (ptrdiff_t)sizeof(mjson_entry_t));
    bdata->id = bjson_id;

    num_parsed = sscanf(context->start, format, &bdata->val_u32);
    assert(num_parsed == 1);

    parsectx_next_token(context);
    return 1;
}

static int parse_key(mjson_parser_t *context)
{
    mjson_entry_t* bdata;
    char*          str_dst;
    const char*    str_src;
    ptrdiff_t      str_len;
    
    assert(context->token==TOK_IDENTIFIER || context->token==TOK_NOESC_STRING);
    
    str_src = context->start;
    str_len = context->next - context->start;

    if (context->token==TOK_NOESC_STRING)
    {
        str_src += 1;
        str_len -= 2;
    }
    
    bdata = (mjson_entry_t*)parsectx_allocate_output(context, (ptrdiff_t)sizeof(mjson_entry_t) + str_len + 1);
    
    if (!bdata) return 0;
    
    bdata->id      = MJSON_ID_UTF8_KEY32;
    bdata->val_u32 = str_len;

    str_dst = (char*)(bdata + 1);
    
    memcpy(str_dst, str_src, str_len);
    str_dst[str_len] = 0;
    
    parsectx_align4_output(context);
    
    parsectx_next_token(context);

    return 1;
}

static int parse_string(mjson_parser_t *context)
{
    mjson_entry_t* bdata;
    char*          str_dst;
    const char*    str_src;
    ptrdiff_t      str_len;

    str_len = context->next - context->start - 2;

    bdata = (mjson_entry_t*)parsectx_allocate_output(context, (ptrdiff_t)sizeof(mjson_entry_t) + str_len + 1);
    
    if (!bdata) return 0;
    
    bdata->id      = MJSON_ID_UTF8_STRING32;
    bdata->val_u32 = str_len;

    str_src = context->start + 1;
    str_dst = (char*)(bdata + 1);
    
    memcpy(str_dst, str_src, str_len);
    str_dst[str_len] = 0;
    
    parsectx_align4_output(context);
    
    parsectx_next_token(context);

    return 1;
}

static int parse_escaped_string(mjson_parser_t *context)
{
#define YYREADINPUT(c) (c>=e?0:*c)
#define YYCTYPE        char
#define YYCURSOR       c
#define YYMARKER       m

    const char* c = context->start+1;
    const char* e = context->next;
    const char* m = NULL;
    const char* s;

    mjson_entry_t* bdata;
    char*            out;
    uint32_t         ch = 0;
    size_t           len;
    int              num_parsed;

    bdata = (mjson_entry_t*)parsectx_allocate_output(context, (ptrdiff_t)sizeof(mjson_entry_t));
    
    if (!bdata) return 0;
    
    bdata->id = MJSON_ID_UTF8_STRING32;

    while (TRUE)
    {
        s = c;

/*!re2c
            CHAR+ {
                out = (char*)parsectx_allocate_output(context, c - s);
                
                if (!out) return 0;
                
                memcpy(out, s, c - s);

                continue;
            }
            
            
            CTL {
                char decoded = s[1];
                
                switch (s[1])
                {
                    case 'b':
                        decoded = '\b';
                        break;
                    case 'n':
                        decoded = '\n';
                        break;
                    case 'r':
                        decoded = '\r';
                        break;
                    case 't':
                        decoded = '\t';
                        break;
                    case 'f':
                        decoded = '\f';
                        break;
                }
                
                out = (char*)parsectx_allocate_output(context, 1);
                
                if (!out) return 0;
                
                *out = decoded;
                
                continue;
            }

            UNICODE {
                out = (char*)parsectx_reserve_output(context, 6);

                if (!out) return 0;

                num_parsed = sscanf(s + 2, "%4x", &ch);
                assert(num_parsed == 1);
                unicode_cp_to_utf8(ch, out, &len);

                parsectx_advance_output(context, len);

                continue;
            }

            "\"" {
                bdata->val_u32 = context->bjson - (uint8_t*)(bdata + 1);
                *context->bjson++ = 0;
                parsectx_align4_output(context);
                parsectx_next_token(context);

                return 1;
            }

            . | "\n" | [\000] { 
                assert(!"reachable");
            }
*/
    }

#undef YYREADINPUT
#undef YYCTYPE           
#undef YYCURSOR          
#undef YYMARKER 

    assert(!"reachable");
    return 0;
}

static int parse_value(mjson_parser_t *context)
{
    uint32_t* id;

    assert(context);
 
    switch (context->token)
    {
        case TOK_NULL:
            id = (uint32_t*)parsectx_allocate_output(context, sizeof(uint32_t));
            if (!id) return 0;
            *id = MJSON_ID_NULL;
            parsectx_next_token(context);
            break;
        case TOK_FALSE:
            id = (uint32_t*)parsectx_allocate_output(context, sizeof(uint32_t));
            if (!id) return 0;
            *id = MJSON_ID_FALSE;
            parsectx_next_token(context);
            break;
        case TOK_TRUE:
            id = (uint32_t*)parsectx_allocate_output(context, sizeof(uint32_t));
            if (!id) return 0;
            *id = MJSON_ID_TRUE;
            parsectx_next_token(context);
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
        case TOK_LEFT_CURLY_BRACKET:
            parsectx_next_token(context);
            if (!parse_key_value_pair(context, TOK_RIGHT_CURLY_BRACKET))
                return 0;
            break;
        case TOK_LEFT_BRACKET:
            parsectx_next_token(context);
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
    mjson_entry_t* array;
    uint8_t*       data_start;
    int            expect_separator;

    assert(context);

    array = (mjson_entry_t*)parsectx_allocate_output(context, sizeof(mjson_entry_t));

    if (!array) return 0;
    
    array->id  = MJSON_ID_ARRAY32;
    data_start = context->bjson;

    expect_separator = FALSE;

    while (context->token != TOK_RIGHT_BRACKET)
    {
        if (expect_separator && context->token == TOK_COMMA)
            parsectx_next_token(context);
        else
            expect_separator = TRUE;

        if (!parse_value(context))
            return 0;
    }

    array->val_u32 = context->bjson - data_start;

    assert((array->val_u32 & 3) == 0);

    parsectx_next_token(context);

    return 1;
}

static int parse_key_value_pair(mjson_parser_t* context, int stop_token)
{
    mjson_entry_t* dictionary;
    uint8_t*       data_start;
    int            expect_separator;
 
    assert(context);

    dictionary = (mjson_entry_t*)parsectx_allocate_output(context, sizeof(mjson_entry_t));
    
    if (!dictionary) return 0;
    
    dictionary->id = MJSON_ID_DICT32;
    data_start     = context->bjson;
    
    expect_separator = FALSE;
    while (context->token != stop_token)
    {
        if (expect_separator && context->token == TOK_COMMA)
            parsectx_next_token(context);
        else
            expect_separator = TRUE;

        switch (context->token)
        {
            case TOK_IDENTIFIER:
            case TOK_NOESC_STRING:
                if (!parse_key(context))
                    return 0;
                break;        
            default:
                return 0;
        }

        if (context->token != TOK_COLON && context->token != TOK_EQUAL)
            return 0;

        parsectx_next_token(context);

        if (!parse_value(context))
            return 0;
    }

    dictionary->val_u32 = context->bjson - data_start;
    
    assert((dictionary->val_u32 & 3) == 0);
    
    parsectx_next_token(context);

    return 1;
}
