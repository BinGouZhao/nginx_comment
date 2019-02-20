#ifndef _NGX_CONFIG_H_INCLUDED_
#define _NGX_CONFIG_H_INCLUDED_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>

typedef struct ngx_pool_s ngx_pool_t;

typedef struct {
	int 		fd;
} ngx_log_t;

#include "ngx_alloc.h"

#ifndef u_char
#define u_char 		unsigned char
#endif

#ifndef ngx_inline
#define ngx_inline      inline
#endif

typedef intptr_t	ngx_int_t;
typedef uintptr_t	ngx_uint_t;
typedef intptr_t	ngx_flag_t;

#define ngx_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
// 地址对齐
#define ngx_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#ifndef NGX_ALIGNMENT
#define NGX_ALIGNMENT   sizeof(unsigned long)    /* platform word */
#endif

#endif
