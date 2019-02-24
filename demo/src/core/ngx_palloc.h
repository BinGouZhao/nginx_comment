#ifndef _NGX_PALLOC_H_INCLUDED_
#define _NGX_PALLOC_H_INCLUDED_

#include <ngx_config.h>
#include <ngx_core.h>

//#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)
#define NGX_MAX_ALLOC_FROM_POOL  4096

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

#define NGX_POOL_ALIGNMENT       16

typedef struct ngx_pool_large_s ngx_pool_large_t;

struct ngx_pool_large_s {
	ngx_pool_large_t		*next;
	void					*alloc;
};


typedef struct {
	u_char					*last;
	u_char					*end;
	ngx_pool_t				*next;
	ngx_uint_t				failed;
} ngx_pool_data_t;

struct ngx_pool_s {
	ngx_pool_data_t			d;
	size_t					max;
	ngx_pool_t				*current;
	//ngx_chain_t			*chain;
	ngx_pool_large_t		*large;
	//ngx_pool_cleanup_t	*cleanup;
	ngx_log_t				*log;
};

ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log);
void ngx_destroy_pool(ngx_pool_t *pool);
void ngx_reset_pool(ngx_pool_t *pool);

void *ngx_palloc(ngx_pool_t *pool, size_t size);
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);

#endif
