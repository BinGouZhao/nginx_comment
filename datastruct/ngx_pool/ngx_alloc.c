
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
#include "ngx_config.h"
#include "ngx_alloc.h"

void *
ngx_alloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = malloc(size);
    if (p == NULL) {
        printf("malloc(%uz) failed", size);
    }

    //ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, log, 0, "malloc: %p:%uz", p, size);

    return p;
}


void *
ngx_calloc(size_t size, ngx_log_t *log)
{
    void  *p;

    p = ngx_alloc(size, log);

    if (p) {
		memset(p, '0', size);
    }

    return p;
}

void *
ngx_memalign(size_t alignment, size_t size, ngx_log_t *log)
{
    void  *p;

    p = memalign(alignment, size);
    if (p == NULL) {
		printf("memalign(%uz, %uz) failed", alignment, size);
    }

    //printf("memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

