
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_ALLOC_H_INCLUDED_
#define _NGX_ALLOC_H_INCLUDED_

#include "ngx_config.h"

void *ngx_alloc(size_t size, ngx_log_t *log);
void *ngx_calloc(size_t size, ngx_log_t *log);

#define ngx_free          free

void *ngx_memalign(size_t alignment, size_t size, ngx_log_t *log);

#endif /* _NGX_ALLOC_H_INCLUDED_ */
