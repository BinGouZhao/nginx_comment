#ifndef _NGX_REDIS_MESSAGE_INCLUDE_H_
#define _NGX_REDIS_MESSAGE_INCLUDE_H_

#include <ngx_config.h>
#include <ngx_core.h>

#define NGX_REDIS_PARSE_INVALID_MESSAGE         10
#define NGX_REDIS_PARSE_INVALID_SUBSCRIBE       11
#define NGX_REDIS_PARSE_INVALID_CHANNEL_NAME    12
#define NGX_REDIS_PARSE_INVALID_LENGTH          13

#define NGX_REDIS_BUFFER_SIZE		1024

typedef struct ngx_redis_connection_s  ngx_redis_connection_t;

struct ngx_redis_connection_s {
	ngx_socket_t		fd;
	ngx_uint_t			state;

	ngx_pool_t			*pool;

	ngx_buf_t			*buffer;

	u_char				*message_start;
	u_char				*message_end;
	u_char				*parse_start;

	u_char				*error_start;
	u_char				*error_end;

	int					message_length;

	ngx_log_t			*log;

	ngx_uint_t			parsed_messages;

	int					channel_name_length;
    ngx_uint_t          channel_name_index;
	char				channel_name[21];

	char				channel_message_prefix[50];
	size_t				channel_message_prefix_n;

    int                 number_length;
	char				number[20];
	ngx_uint_t			number_index;

	unsigned			error:1;
	unsigned			close:1;
	unsigned			after_cr:1;
	unsigned			parse_done:1;
};

ngx_int_t ngx_redis_init_connection(ngx_cycle_t *cycle);
ngx_int_t ngx_redis_subscribe(ngx_redis_connection_t *rc);
ssize_t ngx_redis_read_message(ngx_redis_connection_t *rc);
ngx_int_t ngx_process_parse_message(ngx_redis_connection_t *rc, ngx_buf_t *b);

#endif
