#ifndef _NGX_HASH_H_INCLUDED_
#define _NGX_HASH_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


#define ngx_hash(key, c)   ((ngx_uint_t) key * 31 + c)

typedef struct {
    ngx_uint_t      hash;
    ngx_str_t       key;
    ngx_str_t       value;
    u_char          *lowcase_key;
} ngx_table_elt_t;


#endif
