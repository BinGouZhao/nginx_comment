#include "ngx_config.h"
#include "ngx_palloc.h"

void
ngx_print_pool_use(ngx_pool_t *pool) 
{
	if (pool == NULL) {
		printf("pool is NULL.\n");
		return;
	}

	printf("pool->current: %d, pool->d.last: %d, pool->d.end: %d. \n", 
			pool->current, pool->d.last, pool->d.end);
	return;
}

void main() {
	ngx_pool_t *pool;
	ngx_log_t  *log;

	printf("sizeof ngx_pool_t: %d.\n", sizeof(ngx_pool_t));

	printf("test init 1024 ngx_pool_t.\n");
	pool = ngx_create_pool(1024, log);
	ngx_print_pool_use(pool);

	char *str;
	str = ngx_palloc(pool, 200);
	printf("alloc 200, str address: %d.\n", str);
	ngx_print_pool_use(pool);

	printf("test rest pool.\n");
	ngx_reset_pool(pool);
	ngx_print_pool_use(pool);

	printf("test destroy pool 1000 times.\n");
	ngx_destroy_pool(pool);
	sleep(10);
	printf("start.\n");
	
	int i;
	for (i = 0; i < 1000; i++) {
		pool = ngx_create_pool(1024, log);
		ngx_destroy_pool(pool);
	}

	printf("end.\n");
	sleep(10);
}
