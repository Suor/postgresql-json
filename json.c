#include "postgres.h"
#include "fmgr.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif


#ifdef __FreeBSD__
    #define USE_SYSTEM_MEMMEM
#endif
#ifdef __MACH__
    #define USE_SYSTEM_MEMMEM
#endif


#ifndef USE_SYSTEM_MEMMEM
void* memmem(void* buf, size_t buflen, void* pat, size_t patlen)
{
    void* end = (char*)buf + buflen - patlen;
    while (buf = memchr(buf, *(char*)pat, buflen)) {
        if (buf > end) {
            return NULL;
        }
        if (memcmp(buf, pat, patlen) == 0) {
            return buf;
        }
        buf = (char*)buf+1;
    }
    return NULL;
}
#endif


char *get_value_start(text *json, text *key)
{
    char *pos = (char*)VARDATA(json);
    char *json_end = (char*)json + VARSIZE(json);

    int32 key_size = VARSIZE(key) - VARHDRSZ;
    while (pos < json_end) {
        char *match = (char*)memmem(pos, json_end - pos, VARDATA(key), key_size);
        if (match == NULL) {
            return NULL;
        }

        char *quote_start = match - 1;
        char *quote_end = match + key_size;

        if (
            (quote_start >= (char*)VARDATA(json)) &&
            (*quote_start == '"') &&
            (quote_end < json_end) &&
            (*quote_end == '"')
        ) {
            char *value_pos = quote_end + 1;
            while (value_pos < json_end && (*value_pos == ':' || *value_pos == ' ' ||
                   *value_pos == '\n' || *value_pos == '\r')) value_pos++;
            return value_pos;
        } else {
            pos++;
        }
    }
}


PG_FUNCTION_INFO_V1(json_int);

Datum
json_int(PG_FUNCTION_ARGS)
{
    text *json = PG_GETARG_TEXT_P(0);
    text *key  = PG_GETARG_TEXT_P(1);
    char *pos = get_value_start(json, key);
    if (pos == NULL) {
        PG_RETURN_NULL();
    } else {
        if (memchr("0123456789-", *pos, 11)) {
            PG_RETURN_INT32(atoi(pos));
        } else {
            PG_RETURN_NULL();
        }
    }
}


PG_FUNCTION_INFO_V1(json_bool);

Datum
json_bool(PG_FUNCTION_ARGS)
{
    text *json = PG_GETARG_TEXT_P(0);
    text *key  = PG_GETARG_TEXT_P(1);
    char *pos = get_value_start(json, key);
    if (pos == NULL) {
        PG_RETURN_NULL();
    } else {
        char *json_end = (char*)json + VARSIZE(json);
        if ((json_end - pos >= 4) && memcmp(pos, "true", 4) == 0) {
            PG_RETURN_BOOL(TRUE);
        } else if ((json_end - pos >= 5) && memcmp(pos, "false", 5) == 0) {
            PG_RETURN_BOOL(FALSE);
        } else {
            PG_RETURN_NULL();
        }
    }
}


PG_FUNCTION_INFO_V1(json_string);

Datum
json_string(PG_FUNCTION_ARGS)
{
    text *json = PG_GETARG_TEXT_P(0);
    text *key  = PG_GETARG_TEXT_P(1);
    char *pos = get_value_start(json, key);
    if (pos == NULL) {
        PG_RETURN_NULL();
    } else {
        char *json_end = (char*)json + VARSIZE(json);
        if (*pos == '"') {
            pos = pos + 1;
        } else {
            PG_RETURN_NULL();
        }
        if (pos >= json_end) {
            PG_RETURN_NULL();
        }

        char *pos_end = pos;
        char prev = '"';
        while ((pos_end < json_end) && (*pos_end != '"' || prev == '\\')) {
            prev = *pos_end;
            pos_end++;
        }

        int32 result_size = pos_end - pos + VARHDRSZ;
        text *result = (text*)palloc(result_size);
        SET_VARSIZE(result, result_size);
        memcpy(VARDATA(result), pos, result_size - VARHDRSZ);
        PG_RETURN_TEXT_P(result);
    }
}
