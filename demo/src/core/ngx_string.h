#ifndef _NGX_STRING_H_INCLUDED_
#define _NGX_STRING_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

typedef struct {
    size_t      len;
    u_char      *data;
} ngx_str_t;

#define ngx_string(str) { sizeof(str) - 1, (u_char *) str }
#define ngx_null_string     { 0, NULL }
#define ngx_str_set(str, text)                                               \
    (str)->len = sizeof(text) - 1; (str)->data = (u_char *) text
#define ngx_str_null(str)   (str)->len = 0; (str)->data = NULL

#define ngx_tolower(c)      (u_char) ((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)
#define ngx_toupper(c)      (u_char) ((c >= 'a' && c <= 'z') ? (c & ~0x20) : c)

void ngx_strlow(u_char *dst, u_char *src, size_t n);


#define ngx_strncmp(s1, s2, n)  strncmp((const char *) s1, (const char *) s2, n)


/* msvc and icc7 compile strcmp() to inline loop */
#define ngx_strcmp(s1, s2)  strcmp((const char *) s1, (const char *) s2)


#define ngx_strstr(s1, s2)  strstr((const char *) s1, (const char *) s2)
#define ngx_strlen(s)       strlen((const char *) s)

size_t ngx_strnlen(u_char *p, size_t n);

#define ngx_strchr(s1, c)   strchr((const char *) s1, (int) c)

static ngx_inline u_char *
ngx_strlchr(u_char *p, u_char *last, u_char c)
{
    while (p < last) {

        if (*p == c) {
            return p;
        }

        p++;
    }

    return NULL;
}

#define ngx_cpymem(dst, src, n)   (((u_char *) memcpy(dst, src, n)) + (n))

#define ngx_strlen(s)       strlen((const char *) s)

#define ngx_memcpy(dst, src, n)   (void) memcpy(dst, src, n)

#define ngx_memzero(buf, n)     (void) memset(buf, 0, n)
#define ngx_memset(buf, c, n)     (void) memset(buf, c, n)

u_char *ngx_cpystrn(u_char *dst, u_char *src, size_t n);
u_char *ngx_slprintf(u_char *buf, u_char *last, const char *fmt, ...);
u_char *ngx_sprintf(u_char *buf, const char *fmt, ...);
u_char *ngx_vslprintf(u_char *buf, u_char *last, const char *fmt, va_list args);
u_char *ngx_snprintf(u_char *buf, size_t max, const char *fmt, ...);
 

#define ngx_value_helper(n)   #n
#define ngx_value(n)          ngx_value_helper(n)

#endif /* _NGX_STRING_H_INCLUDED_ */
