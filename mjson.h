/**
 * sjson - fast string based JSON parser/generator library
 * Copyright (C) 2013  Ondřej Jirman <megous@megous.com>
 *
 * WWW: https://github.com/megous/sjson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MJSON_H_INCLUDED__
#define __MJSON_H_INCLUDED__

#include <stdint.h>

struct _mjson_parser_t
{
    int token;
    const char* start;
    const char* next;
    const char* end;
    uint8_t* bjson;
    uint8_t* bjson_limit;
};

struct _mjson_data32_t
{
    uint8_t  id;
    uint32_t data_size;
};

typedef struct _mjson_parser_t mjson_parser_t;
typedef struct _mjson_data32_t mjson_data32_t;

#ifdef __cplusplus
extern "C"
{
#endif

int mjson_parse(const char *json_data, size_t json_data_size, void* bjson_data, size_t bjson_data_size);

#ifdef __cplusplus
}
#endif

#endif
