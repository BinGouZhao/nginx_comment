#ifndef _NGX_BUF_H_INCLUDE_
#define _NGX_BUF_H_INCLUDE_

#include <ngx_config.h>
#include <ngx_core.h>

typedef struct ngx_buf_s    ngx_buf_t;

struct ngx_buf_s {
    u_char          *pos;
    u_char          *last;

    u_char          *start;
    u_char          *end;

    unsigned        temporary:1;
};

ngx_buf_t *ngx_create_temp_buf(ngx_pool_t *pool, size_t size);

#define ngx_alloc_buf(pool)  ngx_palloc(pool, sizeof(ngx_buf_t))
#define ngx_calloc_buf(pool) ngx_pcalloc(pool, sizeof(ngx_buf_t))

#endif
