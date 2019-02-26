#include <ngx_config.h>
#include <ngx_core.h>

volatile ngx_cycle_t  *ngx_cycle;

ngx_cycle_t *
ngx_init_cycle(ngx_cycle_t *old_cycle)
{
    ngx_uint_t              n; //i
    ngx_time_t              *tp;
    ngx_log_t               *log;
    ngx_pool_t              *pool;
    ngx_cycle_t             *cycle;
    char                    hostname[NGX_MAXHOSTNAMELEN];

    ngx_timezone_update();

    tp = ngx_timeofday();
    tp->sec = 0;

    ngx_time_update();

    log = old_cycle->log;

    pool = ngx_create_pool(NGX_CYCLE_POOL_SIZE, log);
    if (pool == NULL) {
        return NULL;
    }
    pool->log = log;

    cycle = ngx_palloc(pool, sizeof(ngx_cycle_t));
    cycle->pool = pool;
    cycle->log = log;
    cycle->old_cycle = old_cycle;

    n = old_cycle->listening.nelts ? old_cycle->listening.nelts : 10;

    if (ngx_array_init(&cycle->listening, pool, n, sizeof(ngx_listening_t))
            != NGX_OK)
    {
        ngx_destroy_pool(pool);
        return NULL;
    }

    ngx_memzero(cycle->listening.elts, n * sizeof(ngx_listening_t));

    ngx_queue_init(&cycle->reusable_connections_queue);

    if (gethostname(hostname, NGX_MAXHOSTNAMELEN) == -1) {
        ngx_log_error(NGX_LOG_EMERG, log, ngx_errno, "gethostname() failed");
        ngx_destroy_pool(pool);
        return NULL;
    }

    hostname[NGX_MAXHOSTNAMELEN - 1] = '\0';
    cycle->hostname.len = ngx_strlen(hostname);

    cycle->hostname.data = ngx_pnalloc(pool, cycle->hostname.len);
    if (cycle->hostname.data == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }

    ngx_strlow(cycle->hostname.data, (u_char *) hostname, cycle->hostname.len);

    if (ngx_log_open_default(cycle) != NGX_OK) {
        //goto failed;
        return NULL;
    }

    cycle->log = &cycle->new_log;
    pool->log = &cycle->new_log;

    struct sockaddr_in address;
    ngx_memzero(&address, sizeof(address));
    address.sin_family =AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    address.sin_port = htons(40199);

    ngx_listening_t *ls = ngx_create_listening(cycle, (struct sockaddr *)&address, sizeof(address));

    cycle->listening.elts = ls;
    cycle->listening.nelts = 1;

    if (ngx_open_listening_sockets(cycle) != NGX_OK) {
        //goto failed;
        return NULL;
    }


    return NULL;
}




