#include <string.h>
#include "sput.h"
#include "mjson.h"

void mjson_valid_syntax_tests();
void mjson_invalid_syntax_tests();
void mjson_content_tests();

int main()
{
    sput_start_testing();

    sput_enter_suite("mjson: Parsing tests");
    sput_run_test(mjson_valid_syntax_tests);
    sput_run_test(mjson_invalid_syntax_tests);

    sput_enter_suite("mjson: Data tests");
    sput_run_test(mjson_content_tests);

    sput_finish_testing();

    return sput_get_return_value();
}

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

static const int MAX_BJSON_SIZE = 1*1024*1024;
uint8_t bjson[MAX_BJSON_SIZE];

const char* valid_json[] =
{
    "",
    " \n\t \t \n  \n",
    "a:1\nb=2",
    "{}",
    "{a:1}\n",
    "{a:1 b:2}\n",
    "{a=1 b:2}\n",
    "{a=1 /*Comment*/ b:2}\n",
    "{a=1 /*Comme* /nt*/b:2}\n",
    "{a=1 /*Comme* nt*/ b:2}\n",
    "{a=1 /*Co*m/me* nt*/b:2}\n",
    "{a=1 /****Comment*/ b:2}\n",
    "{a=1 /*Comment****/b:2}\n",
    "{a=1 /*Comment\n\n*/b:2}\n",
    "{a:1 //Comment\n}\n",
    "{a:\"// comment\n\"}\n",
    "{a:\"/* comment */\"}\n",
    "{\"a\":1}\n",
    "{\"a\": 1}\n",
    "{\"a\": [1, 2, {}]}",
    "{\"a\": [1, 2,\n[{\t}]\n], \"b\": [1, 2, {}]}",

    "{\"a\": []}",
    "{\"a\": [1]}",
    "{\"a\": [1,2,3]}",
    "{\"a\": [[1, [2], [[]]], 3]}",
    "{\"a\": [[1, 2], 3]}",
    "{\"a\":[00]}",

    "[]",
    "[{}]",
    "[1]",
    "[1,2,3]",
    "[[1, [2], [[]]], 3]",
    "[[1, 2], 3]",
    "[00]",

    "{\"a\": null}",
    "{\"a\": true}",
    "{\"a\": false}",
    "{\"a\": 0}",
    "{\"a\": 1}",
    "{\"a\": -10}",
    "{\"a\": 1.128879}",
    "{\"a\": 1.457e-10}",
    "{\"a\": -1.457e+10}",
    "{\"a\": 1.457e10}",
    "{\"a\": 1e10}",
    "[0x577fd]",
    "[0577]",
    "{\"a\":00}",
    "{\"a\":01}",
    "{\"a\":.1}",
    "{\"a\":   -1e+10}",
    "{\"a\":   -1E+10}",
    "{\"a\":  \"\" }",
    "{\"a\": \"qweqwe\"}",
    "{\"a\": \"\\\\\\\"\\n\\r\\b\\f\\t\\u1234\"}",

    "{\n\"name\": \"Jack (\\\"Bee\\\") Nimble\", \n\"format\": {\"type\":       \"rect\", \n\"width\":      1920, \n\"height\":     1080, \n\"interlace\":  false,\"frame rate\": 24\n}\n}",
    "[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]",
    "[\n    [0, -1, 0],\n    [1, 0, 0],\n    [0, 0, 1]\n       ]\n",
    "{\n               \"Image\": {\n                  \"Width\":  800,\n                      \"Height\": 600,\n                      \"Title\":  \"View from 15th Floor\",\n                 \"Thumbnail\": {\n                              \"Url\":    \"http:/*www.example.com/image/481989943\",\n                               \"Height\": 125,\n                              \"Width\":  \"100\"\n                   },\n                    \"IDs\": [116, 943, 234, 38793]\n               }\n     }",
    "[\n        {\n     \"precision\": \"zip\",\n       \"Latitude\":  37.7668,\n       \"Longitude\": -122.3959,\n     \"Address\":   \"\",\n  \"City\":      \"SAN FRANCISCO\",\n     \"State\":     \"CA\",\n        \"Zip\":       \"94107\",\n     \"Country\":   \"US\"\n         },\n    {\n     \"precision\": \"zip\",\n       \"Latitude\":  37.371991,\n     \"Longitude\": -122.026020,\n   \"Address\":   \"\",\n  \"City\":      \"SUNNYVALE\",\n         \"State\":     \"CA\",\n        \"Zip\":       \"94085\",\n     \"Country\":   \"US\"\n         }\n     ]",

    /* Empty object. */
    "{  }",  
    /* Empty array. */
    "[  ]",  
    /* Nesting. */
    "[ {  }, {  }, [ [  ], {  } ] ]",   
    /* ASCII String. */ 
    "{ \"ascii\": \"This is an ASCII string.\" }",
    /* UNICODE String. */
    "{ \"unicode\": \"This is a \\uDEAD\\uBEEF.\" }",
    /* UTF-8 String. */
    "{ \"utf8\": \"蘊\" }",
    /* Integer. */ 
    "{ \"int\": 12439084123 }",
    /* Exp. */ 
    "{ \"exp\": -12439084123E+1423 }",
    /* Frac. */ 
    "{ \"frac\": 12439084123.999e-1423 }",
    /* Boolean. */
    "[ true, false ]",
    /* Null. */
    "{ \"NullMatrix\": [ [ null, null ], [ null, null ] ] }",
    /*Ukrainian*/
    "{ \"Мова\": \"Українська\" }",
    /*Japanese*/
    "{ \"言語\": \"日本語\" }",
};

void mjson_valid_syntax_tests()
{
    mjson_element_t top_element;

    for (int i = 0; i < ARRAY_SIZE(valid_json); ++i)
    {
        sput_fail_unless(mjson_parse(valid_json[i], strlen(valid_json[i]), bjson, sizeof(bjson), &top_element), "");
    }
}

const char* invalid_json[] =
{
    "{",
    "}",
    "{}{}",
    "[]{}",
    "{}[]",
    "{a=1 /*Com/*ment*/ */b:2}\n",
    "{,a\":1}",
    "{,}",
    "{a\":1,}",
    "{\"a:1}",
    "{\"a\":1,}",
    "{\"a\":,}",
    "{\"a\":, \"b\":1}",
    "{\"a\":}",
    "{\"a\"}",
    "{\"a\":1}1",
    "{\"a\":1, \"x\": }",
    "{\"a\":1, \"x\": }}",
    "{1:1}",
    "{\"a\":1 1}",

    "{\"a\":}",
    "{\"a\":  \n\t }",
    "{\"a\":nullnull}",
    "{\"a\":n}",
    ",",
    "[0x577fdhj]",
    "[67ab]",
    "a:0x6788dfgh:3",
    "a:0x6788df_gh:3",
    "[0589]",
    "[0x577fd0xfd]",
    "{\"a\":truetrue}",
    "{\"a\":TRUE}",
    "{\"a\":FALSE}",
    "{\"a\":0 1}",
    "{\"a\":1e-}",
    "{\"a\":.}",
    "{\"a\":\"}",
    "{\"a\":\"\"\"\"}",
    "{\"a\":\"\"\"}",
    "{\"a\":a:\"\"}",
    "{\"a\":\"a\":\"\"}",

    "{\"a\":[}",
    "{\"a\":]}",
    "{\"a\":[,}",
    "{\"a\":[0}",
    "{\"a\":[0,}",
    "{\"a\":[0,}",
    "{\"a\":[0,1}",
    "{\"a\":[,]}",
    "{\"a\":[0,]}",
    "{\"a\":[,0]}",
    "{\"a\":[]0}",

    "[",
    "]",
    "[,",
    "[0",
    "[0,",
    "[0,",
    "[0,1",
    "[,]",
    "[0,]",
    "[,0]",
    "[]0",
};

void mjson_invalid_syntax_tests()
{
    mjson_element_t top_element;

    for (int i = 0; i < ARRAY_SIZE(invalid_json); ++i)
    {
        sput_fail_if(mjson_parse(invalid_json[i], strlen(invalid_json[i]), bjson, sizeof(bjson), &top_element), "");
    }
}

const char* jsonAPItest = 
    "a = 5\n"
    "b = \"string\"\n"
    "c = 3.0\n"
    "k : {\n"
    "   d:4\n"
    "   ss=\"escaped\\n\"\n"
    "   k:2.0\n"
    "}\n"
    "t = true\n"
    "array:\n"
    "[\n"
    "43\n"
    "122.0\n"
    "\"String\"\n"
    "]\n"
    "c : null\n"
    "ff = 11.0\n";

void mjson_content_tests()
{
    int result;
    mjson_element_t top_element;
    
    result = mjson_parse(jsonAPItest, strlen(jsonAPItest), bjson, sizeof(bjson), &top_element);
    sput_fail_unless(result, "");

    mjson_element_t it, v, it2, top2;
    int ires, bres;
    const char* cres;
    float fres;

    it = mjson_get_member_first(top_element, &v);
    sput_fail_unless(it && v, "");
    ires = mjson_get_int(v, 0);
    sput_fail_unless(ires == 5, "");

    it = mjson_get_member_next(top_element, it, &v);
    sput_fail_unless(it && v, "");
    cres = mjson_get_string(v, "");
    sput_fail_unless(strcmp(cres, "string")==0, "");

    it = mjson_get_member_next(top_element, it, &v);
    sput_fail_unless(it && v, "");
    fres = mjson_get_float(v, 0.0f);
    sput_fail_unless(fres == 3.0f, "");

    it = mjson_get_member_next(top_element, it, &top2);
    sput_fail_unless(it && v, "");

    it2 = mjson_get_member_first(top2, &v);
    sput_fail_unless(it2 && v, "");
    ires = mjson_get_int(v, 0);
    sput_fail_unless(ires == 4, "");

    it2 = mjson_get_member_next(top2, it2, &v);
    sput_fail_unless(it && v, "");
    cres = mjson_get_string(v, "");
    sput_fail_unless(strcmp(cres, "escaped\n")==0, "");

    it2 = mjson_get_member_next(top2, it2, &v);
    sput_fail_unless(it2 && v, "");
    fres = mjson_get_float(v, 0.0f);
    sput_fail_unless(fres == 2.0f, "");

    it = mjson_get_member_next(top_element, it, &v);
    sput_fail_unless(it && v, "");
    bres = mjson_get_bool(v, false);
    sput_fail_unless(bres, "");

    it = mjson_get_member_next(top_element, it, &top2);
    sput_fail_unless(it && v, "");
    
    it2 = mjson_get_element_first(top2);
    sput_fail_unless(it2, "");
    ires = mjson_get_int(it2, 0);
    sput_fail_unless(ires == 43, "");
    
    it2 = mjson_get_element_next(top2, it2);
    sput_fail_unless(it2, "");
    fres = mjson_get_float(it2, 0.0f);
    sput_fail_unless(fres == 122.0f, "");
    
    it2 = mjson_get_element_next(top2, it2);
    sput_fail_unless(it, "");
    cres = mjson_get_string(it2, "");
    sput_fail_unless(strcmp(cres, "String")==0, "");

    it = mjson_get_member_next(top_element, it, &v);
    sput_fail_unless(it && v, "");
    bres = mjson_is_null(v);
    sput_fail_unless(bres, "");

    it = mjson_get_member_next(top_element, it, &v);
    sput_fail_unless(it && v, "");
    fres = mjson_get_float(v, 0.0f);
    sput_fail_unless(fres == 11.0f, "");
}
