#include <ngx_config.h>
#include <ngx_core.h>

static ngx_inline void *ngx_palloc_small(ngx_pool_t *pool, size_t size,
    ngx_uint_t align);
static void *ngx_palloc_block(ngx_pool_t *pool, size_t size);
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size);

ngx_pool_t *
ngx_create_pool(size_t size, ngx_log_t *log)
{
	ngx_pool_t	*p;

	p = ngx_memalign(NGX_POOL_ALIGNMENT, size, log);
	if (p == NULL) {
		return NULL;
	}

	// 
	p->d.last = (u_char *) p + sizeof(ngx_pool_t);
	p->d.end = (u_char *) p + size;
	p->d.next = NULL;
	p->d.failed = 0;

	size = size - sizeof(ngx_pool_t);
	p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

	p->current = p;
//	p->chain = NULL;
    p->large = NULL;
//  p->cleanup = NULL;
	p->log = log;

    return p;
}

void 
ngx_destroy_pool(ngx_pool_t *pool)
{
	ngx_pool_t			*p, *n;
	ngx_pool_large_t	*l;
	//ngx_pool_cleanup_t	*c;
	
	// 释放大块内存 
	for (l = pool->large; l; l = l->next) {
		if (l->alloc) {
			ngx_free(l->alloc);
		}
	}

	// 释放小块内存(单链)
	for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
		ngx_free(p);

		if (n == NULL) {
			break;
		}
	}
}	

// 重置内存池
// 释放大内存块, 重置小内存块
void ngx_reset_pool(ngx_pool_t *pool)
{
	ngx_pool_t			*p;
	ngx_pool_large_t	*l;

	for (l = pool->large; l; l = l->next) {
		if (l->alloc) {
			ngx_free(l->alloc);
		}
	}

	for (p = pool; p; p = p->d.next) {
		p->d.last = (u_char *) p + sizeof(ngx_pool_t);
		p->d.failed = 0;
	}

	pool->current = pool;
//	pool->chain = NULL;
   	pool->large = NULL;
}	

void *
ngx_palloc(ngx_pool_t *pool, size_t size)
{
	if (size <= pool->max) {
		return ngx_palloc_small(pool, size, 1);
	}

	return ngx_palloc_large(pool, size);
}

static ngx_inline void *
ngx_palloc_small(ngx_pool_t *pool, size_t size, ngx_uint_t align)
{
	u_char 		*m;
	ngx_pool_t 	*p;

	p = pool->current;

	do {
		m = p->d.last;

		// 地址对齐
		if (align) {
			m = ngx_align_ptr(m, NGX_ALIGNMENT);
		}

		// 从 ngx_pool_data_t 中分配
		if ((size_t) (p->d.end - m) >= size) {
			p->d.last = m + size;

			return m;
		}

		p = p->d.next;
	} while (p);

	return ngx_palloc_block(pool, size);
}

// 重新申请一块 pool 大小的内存
static void *
ngx_palloc_block(ngx_pool_t *pool, size_t size)
{
	u_char 			*m;
	size_t			psize;
	ngx_pool_t		*p, *new;

	psize = (size_t) (pool->d.end - (u_char *) pool);

	m = ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log);
	if (m == NULL) {
		return NULL;
	}

	new = (ngx_pool_t *)m;

	new->d.end = m + psize;
	new->d.next = NULL;
	new->d.failed = 0;

	m += sizeof(ngx_pool_data_t);
	m = ngx_align_ptr(m, NGX_ALIGNMENT);
	new->d.last = m + size;

	for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->current = p->d.next;
        }
    }

    p->d.next = new;

    return m;
}

static void *
ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
	void 				*p;
	ngx_uint_t			n;
	ngx_pool_large_t	*large;

	p = ngx_alloc(size, pool->log);
	if (p == NULL) {
		return NULL;
	}

	n = 0;

	for (large = pool->large; large; large = large->next) {
		if (large == NULL) {
			large->alloc = p;
			return p;
		}

		if (n++ > 3) {
			break;
		}
	}

	large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
	if (large == NULL) {
		ngx_free(p);
		return NULL;
	}

	large->alloc = p;
	large->next = pool->large;
	pool->large = large;

	return p;
}

void *
ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
    void *p;

    p = ngx_palloc(pool, size);
    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}

void *
ngx_pnalloc(ngx_pool_t *pool, size_t size)
{
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 0);
    }

    return ngx_palloc_large(pool, size);
}
