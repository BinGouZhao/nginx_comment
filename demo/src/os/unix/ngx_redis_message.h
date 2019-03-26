#ifndef _NGX_REDIS_MESSAGE_INCLUDE_H_
#define _NGX_REDIS_MESSAGE_INCLUDE_H_

#define NGX_REDIS_BUFFER_SIZE		1024

struct ngx_redis_connection_s {
	ngx_socket_t		fd;
	ngx_uint_t			state;

	ngx_pool_t			*pool;

	ngx_buf_t			*buffer;

	u_char				*message_start;
	u_char				*message_end;

	u_char				*error_start;
	u_char				*error_end;

	int					message_length;

	ngx_log_t			*log;

	ngx_uint_t			parsed_messages;

	int					channel_name_length;
	u_char				channel_name[21];

	u_char				channel_message_prefix[50];
	size_t				channel_message_prefix_n;

	u_char				number[20];
	ngx_uint_t			number_index;

	unsigned			error:1;
	unsigned			close:1;
	unsigned			after_cr;
};

#endif
