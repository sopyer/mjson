#include <string.h>
#include "mjson.h"


void mjson_syntax_tests();

int main()
{
    mjson_syntax_tests();

    return 0;
}

#include <assert.h>

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

struct syntax_validity_test_record_t
{
    int         expected_result;
    const char* json;
};

syntax_validity_test_record_t tests[] = 
{
    1, "",
    1, " \n\t \t \n  \n",
    1, "a:1\nb=2",
    1, "{}",
    1, "{a:1}\n",
    1, "{a:1 b:2}\n",
    1, "{a=1 b:2}\n",
    1, "{a=1 /*Comment*/ b:2}\n",
    1, "{a=1 /*Comme* /nt*/b:2}\n",
    1, "{a=1 /*Comme* nt*/ b:2}\n",
    1, "{a=1 /*Co*m/me* nt*/b:2}\n",
    1, "{a=1 /****Comment*/ b:2}\n",
    1, "{a=1 /*Comment****/b:2}\n",
    1, "{a=1 /*Comment\n\n*/b:2}\n",
    1, "{a:1 //Comment\n}\n",
    1, "{a:\"// comment\n\"}\n",
    1, "{a:\"/* comment */\"}\n",
    1, "{\"a\":1}\n",
    1, "{\"a\": 1}\n",
    1, "{\"a\": [1, 2, {}]}",
    1, "{\"a\": [1, 2,\n[{\t}]\n], \"b\": [1, 2, {}]}",

    1, "{\"a\": []}",
    1, "{\"a\": [1]}",
    1, "{\"a\": [1,2,3]}",
    1, "{\"a\": [[1, [2], [[]]], 3]}",
    1, "{\"a\": [[1, 2], 3]}",
    1, "{\"a\":[00]}",

    1, "[]",
    1, "[{}]",
    1, "[1]",
    1, "[1,2,3]",
    1, "[[1, [2], [[]]], 3]",
    1, "[[1, 2], 3]",
    1, "[00]",

    1, "{\"a\": null}",
    1, "{\"a\": true}",
    1, "{\"a\": false}",
    1, "{\"a\": 0}",
    1, "{\"a\": 1}",
    1, "{\"a\": -10}",
    1, "{\"a\": 1.128879}",
    1, "{\"a\": 1.457e-10}",
    1, "{\"a\": -1.457e+10}",
    1, "{\"a\": 1.457e10}",
    1, "{\"a\": 1e10}",
    1, "[0x577fd]",
    1, "[0577]",
    1, "{\"a\":00}",
    1, "{\"a\":01}",
    1, "{\"a\":.1}",
    1, "{\"a\":   -1e+10}",
    1, "{\"a\":   -1E+10}",
    1, "{\"a\":  \"\" }",
    1, "{\"a\": \"qweqwe\"}",
    1, "{\"a\": \"\\\\\\\"\\n\\r\\b\\f\\t\\u1234\"}",

    0, "{",
    0, "}",
    0, "{}{}",
    0, "[]{}",
    0, "{}[]",
    0, "{a=1 /*Com/*ment*/ */b:2}\n",
    0, "{,a\":1}",
    0, "{,}",
    0, "{a\":1,}",
    0, "{\"a:1}",
    0, "{\"a\":1,}",
    0, "{\"a\":,}",
    0, "{\"a\":, \"b\":1}",
    0, "{\"a\":}",
    0, "{\"a\"}",
    0, "{\"a\":1}1",
    0, "{\"a\":1, \"x\": }",
    0, "{\"a\":1, \"x\": }}",
    0, "{1:1}",
    0, "{\"a\":1 1}",

    0, "{\"a\":}",
    0, "{\"a\":  \n\t }",
    0, "{\"a\":nullnull}",
    0, "{\"a\":n}",
    0, ",",
    0, "[0x577fdhj]",
    0, "[67ab]",
    0, "a:0x6788dfgh:3",
    0, "a:0x6788df_gh:3",
    0, "[0589]",
    0, "[0x577fd0xfd]",
    0, "{\"a\":truetrue}",
    0, "{\"a\":TRUE}",
    0, "{\"a\":FALSE}",
    0, "{\"a\":0 1}",
    0, "{\"a\":1e-}",
    0, "{\"a\":.}",
    0, "{\"a\":\"}",
    0, "{\"a\":\"\"\"\"}",
    0, "{\"a\":\"\"\"}",
    0, "{\"a\":a:\"\"}",
    0, "{\"a\":\"a\":\"\"}",

    0, "{\"a\":[}",
    0, "{\"a\":]}",
    0, "{\"a\":[,}",
    0, "{\"a\":[0}",
    0, "{\"a\":[0,}",
    0, "{\"a\":[0,}",
    0, "{\"a\":[0,1}",
    0, "{\"a\":[,]}",
    0, "{\"a\":[0,]}",
    0, "{\"a\":[,0]}",
    0, "{\"a\":[]0}",

    0, "[",
    0, "]",
    0, "[,",
    0, "[0",
    0, "[0,",
    0, "[0,",
    0, "[0,1",
    0, "[,]",
    0, "[0,]",
    0, "[,0]",
    0, "[]0",

    1, "{\n\"name\": \"Jack (\\\"Bee\\\") Nimble\", \n\"format\": {\"type\":       \"rect\", \n\"width\":      1920, \n\"height\":     1080, \n\"interlace\":  false,\"frame rate\": 24\n}\n}",
    1, "[\"Sunday\", \"Monday\", \"Tuesday\", \"Wednesday\", \"Thursday\", \"Friday\", \"Saturday\"]",
    1, "[\n    [0, -1, 0],\n    [1, 0, 0],\n    [0, 0, 1]\n       ]\n",
    1, "{\n               \"Image\": {\n                  \"Width\":  800,\n                      \"Height\": 600,\n                      \"Title\":  \"View from 15th Floor\",\n                 \"Thumbnail\": {\n                              \"Url\":    \"http:/*www.example.com/image/481989943\",\n                               \"Height\": 125,\n                              \"Width\":  \"100\"\n                   },\n                    \"IDs\": [116, 943, 234, 38793]\n               }\n     }",
    1, "[\n        {\n     \"precision\": \"zip\",\n       \"Latitude\":  37.7668,\n       \"Longitude\": -122.3959,\n     \"Address\":   \"\",\n  \"City\":      \"SAN FRANCISCO\",\n     \"State\":     \"CA\",\n        \"Zip\":       \"94107\",\n     \"Country\":   \"US\"\n         },\n    {\n     \"precision\": \"zip\",\n       \"Latitude\":  37.371991,\n     \"Longitude\": -122.026020,\n   \"Address\":   \"\",\n  \"City\":      \"SUNNYVALE\",\n         \"State\":     \"CA\",\n        \"Zip\":       \"94085\",\n     \"Country\":   \"US\"\n         }\n     ]",
};

static const int MAX_BJSON_SIZE = 1*1024*1024;

const char* jsonAPItest = 
    "a = 5\n"
    "b = \"string\"\n"
    "c = 3.0\n"
    "k : {\n"
    "   d:4\n"
    "   k:2.0\n"
    "}\n"
    "t = true\n"
    "c : null\n"
    "ff = 11.0\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n"
    "\n";

uint8_t bjson[MAX_BJSON_SIZE];

void mjson_syntax_tests()
{
    int i;
    mjson_element_t top_element;
    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        assert(mjson_parse(tests[i].json, strlen(tests[i].json), bjson, sizeof(bjson), &top_element) == tests[i].expected_result);
    }

    int result;
    
    result = mjson_parse(jsonAPItest, strlen(jsonAPItest), bjson, sizeof(bjson), &top_element);
    assert(result);

    mjson_element_t it, v, it2, top2;
    int ires, bres;
    const char* cres;
    float fres;

    it = mjson_get_member_first(top_element, &v);
    assert(it && v);
    ires = mjson_get_int(v, 0);
    assert(ires == 5);

    it = mjson_get_member_next(top_element, it, &v);
    assert(it && v);
    cres = mjson_get_string(v, "");
    assert(strcmp(cres, "string")==0);

    it = mjson_get_member_next(top_element, it, &v);
    assert(it && v);
    fres = mjson_get_float(v, 0.0f);
    assert(fres == 3.0f);

    it = mjson_get_member_next(top_element, it, &top2);
    assert(it && v);

    it2 = mjson_get_member_first(top2, &v);
    assert(it2 && v);
    ires = mjson_get_int(v, 0);
    assert(ires == 4);

    it2 = mjson_get_member_next(top2, it2, &v);
    assert(it2 && v);
    fres = mjson_get_float(v, 0.0f);
    assert(fres == 2.0f);

    it = mjson_get_member_next(top_element, it, &v);
    assert(it && v);
    bres = mjson_get_bool(v, false);
    assert(bres);

    it = mjson_get_member_next(top_element, it, &v);
    assert(it && v);
    bres = mjson_is_null(v);
    assert(bres);

    it = mjson_get_member_next(top_element, it, &v);
    assert(it && v);
    fres = mjson_get_float(v, 0.0f);
    assert(fres == 11.0f);
}
