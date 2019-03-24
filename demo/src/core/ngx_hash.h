#ifndef _NGX_HASH_H_INCLUDED_
#define _NGX_HASH_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

#define ngx_hash(key, c)        ((ngx_uint_t) key * 31 + c)
#define ngx_hash_table_null     ((ngx_hash_table_value) 0)

//typedef ngx_websocket_connection_t  ngx_hash_table_value;
typedef ngx_uint_t                  ngx_hash_table_value;
typedef ngx_uint_t                  ngx_hash_table_key;

typedef ngx_uint_t (*ngx_hash_table_hash_func_pt) (ngx_hash_table_key value);
typedef int (*ngx_hash_table_equal_funct_pt) (ngx_hash_table_key v1, ngx_hash_table_key v2);

typedef struct {
    ngx_uint_t      hash;
    ngx_str_t       key;
    ngx_str_t       value;
    u_char          *lowcase_key;
} ngx_table_elt_t;

typedef struct ngx_hash_table_pair_s    ngx_hash_table_pair_t;
typedef struct ngx_hash_table_entry_s   ngx_hash_table_entry_t;
typedef struct ngx_hash_table_s         ngx_hash_table_t;

struct ngx_hash_table_pair_s {
    ngx_hash_table_key          key;
    ngx_hash_table_value        value;
};

struct ngx_hash_table_entry_s {
    ngx_hash_table_pair_t       pair;
    ngx_hash_table_entry_t      *next;
};

struct ngx_hash_table_s {
    ngx_hash_table_entry_t          **table;
    ngx_hash_table_hash_func_pt     hash_func;
    ngx_hash_table_equal_funct_pt   equal_func;
    ngx_uint_t                      table_size;
    ngx_uint_t                      entries;
    ngx_uint_t                      prime_index;
};

ngx_hash_table_t *ngx_hash_table_new(ngx_hash_table_hash_func_pt hash_func,
        ngx_hash_table_equal_funct_pt equal_func);

int ngx_hash_table_insert(ngx_hash_table_t *hash_table, ngx_hash_table_key key,
        ngx_hash_table_value value);

ngx_hash_table_value ngx_hash_table_lookup(ngx_hash_table_t *hash_table,
        ngx_hash_table_key key);

int ngx_hash_table_remove(ngx_hash_table_t *hash_table, ngx_hash_table_key key);

#endif
