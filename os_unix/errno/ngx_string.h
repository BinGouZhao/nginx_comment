#ifndef _NGX_STRING_H_INCLUDED_
#define _NGX_STRING_H_INCLUDED_

#include "ngx_config.h"
#include "ngx_core.h"

typedef struct {
    size_t      len;
    u_char      *data;
} ngx_str_t;

#define ngx_string(str) { sizeof(str) - 1, (u_char *) str }

#define ngx_cpymem(dst, src, n)   (((u_char *) memcpy(dst, src, n)) + (n))

#define ngx_strlen(s)       strlen((const char *) s)

#define ngx_memcpy(dst, src, n)   (void) memcpy(dst, src, n)

#endif /* _NGX_STRING_H_INCLUDED_ */
