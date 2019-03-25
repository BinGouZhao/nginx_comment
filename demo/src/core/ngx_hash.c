#include <ngx_config.h>
#include <ngx_core.h>

static int ngx_hash_table_enlarge(ngx_hash_table_t *hash_table);

static const unsigned int hash_table_primes[] = {
	193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317,
	196613, 393241, 786433, 1572869, 3145739, 6291469,
	12582917, 25165843, 50331653, 100663319, 201326611,
	402653189, 805306457, 1610612741,
};

static const ngx_uint_t hash_table_num_primes
    = sizeof(hash_table_primes) / sizeof(int);

static ngx_int_t
ngx_hash_table_allocate_table(ngx_hash_table_t *hash_table)
{
    ngx_uint_t      new_table_size;

    if (hash_table->prime_index < hash_table_num_primes) {
        new_table_size = hash_table_primes[hash_table->prime_index];
    } else {
        new_table_size = hash_table->entries * 10;
    }

    hash_table->table_size = new_table_size;

    hash_table->table = calloc(hash_table->table_size,
                               sizeof(ngx_hash_table_entry_t *));

    return hash_table->table != NULL;
}

ngx_hash_table_t *
ngx_hash_table_new(ngx_hash_table_hash_func_pt hash_func,
                   ngx_hash_table_equal_funct_pt equal_func)
{
    ngx_hash_table_t    *hash_table;

    hash_table = (ngx_hash_table_t *) malloc(sizeof(ngx_hash_table_t));
    if (hash_table == NULL) {
        return NULL;
    }

    hash_table->hash_func = hash_func;
    hash_table->equal_func = equal_func;
    hash_table->entries = 0;
    hash_table->prime_index = 0;

    if (!ngx_hash_table_allocate_table(hash_table)) {
        free(hash_table);

        return NULL;
    }

    return hash_table;
}

static int 
ngx_hash_table_enlarge(ngx_hash_table_t *hash_table)
{
    ngx_uint_t              i, index, old_table_size, old_prime_index;
    ngx_hash_table_entry_t  **old_table, *rover, *next;
    ngx_hash_table_pair_t   *pair;

    old_table = hash_table->table;
    old_table_size = hash_table->table_size;
    old_prime_index = hash_table->prime_index;

    ++hash_table->prime_index;

    if (!ngx_hash_table_allocate_table(hash_table)) {

        hash_table->table = old_table;
        hash_table->table_size = old_table_size;
        hash_table->prime_index = old_prime_index;

        return 0;
    }

    for (i = 0; i < old_table_size; i++) {

        rover = old_table[i];

        while (rover != NULL) {
            next = rover->next;

            pair = &(rover->pair);

            index = hash_table->hash_func(pair->key) % hash_table->table_size;

            rover->next = hash_table->table[index];
            hash_table->table[index] = rover;

            rover = next;
        }
    }

    free(old_table);

    return 1;
}

int
ngx_hash_table_insert(ngx_hash_table_t *hash_table, ngx_hash_table_key key,
                      ngx_hash_table_value value)
{
    ngx_uint_t              index;
    ngx_hash_table_pair_t   *pair;
    ngx_hash_table_entry_t  *rover, *newentry;
                                    
    if ((hash_table->entries * 3) / hash_table->table_size > 0) {
        
        if (!ngx_hash_table_enlarge(hash_table)) {

            return 0;
        }
    }

    index = hash_table->hash_func(key) % hash_table->table_size;

    rover = hash_table->table[index];

    while (rover != NULL) {

        pair = &(rover->pair);

        if (hash_table->equal_func(pair->key, key) != 0) {

            pair->key = key;
            pair->value = value;

            return 1;
        }

        rover = rover->next;
    }

    newentry = malloc(sizeof(ngx_hash_table_entry_t));
    if (newentry == NULL) {
        return 0;
    }

    newentry->pair.key = key;
    newentry->pair.value = value;

    newentry->next = hash_table->table[index];
    hash_table->table[index] = newentry;

    ++hash_table->entries;

    return 1;
}

ngx_hash_table_value 
ngx_hash_table_lookup(ngx_hash_table_t *hash_table, ngx_hash_table_key key)
{
    ngx_uint_t                  index;
    ngx_hash_table_entry_t      *rover;
    ngx_hash_table_pair_t       *pair;

    index = hash_table->hash_func(key) % hash_table->table_size;

    rover = hash_table->table[index];

    while (rover != NULL) {
        pair = &(rover->pair);

        if (hash_table->equal_func(key, pair->key) != 0) {

            return pair->value;
        }

        rover = rover->next;
    }

    return ngx_hash_table_null;
}

#if 0

ngx_uint_t
ngx_int_hash_func(ngx_uint_t value)
{
    return value;
}

int 
ngx_int_hash_equel_func(ngx_uint_t v1, ngx_uint_t v2)
{
    return v1 == v2;
}

#define NGX_TEST        1000000

int main() {
    ngx_uint_t          i, ret;
    ngx_hash_table_t    *hash_table;

    hash_table = ngx_hash_table_new(ngx_int_hash_func, ngx_int_hash_equel_func);

    for (i = 1; i < NGX_TEST; i++) {
        ret = ngx_hash_table_insert(hash_table, i, i);
        if (ret == 0) {
            printf("hash insert failed.\n");
            return 1;
        }
    }

    for (i = 1; i < NGX_TEST; i++) {
        ret = ngx_hash_table_lookup(hash_table, i);

        if (ret != i) {
            printf("hash lookup failed.\n");
            return 1;
        }
    }

    printf("hash test success.\n");
    return 0;
}

int 
ngx_hash_table_remove(ngx_hash_table_t *hash_table, ngx_hash_table_key key)
{
}
#endif
                
