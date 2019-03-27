#include <ngx_config.h>
#include <ngx_core.h>

char		*ngx_redis_ip = "127.0.0.1";
uint16_t	ngx_redis_port = 6379;

ngx_int_t
ngx_redis_init_connection(ngx_cycle_t *cycle)
{
	ngx_socket_t				s;
	ngx_buf_t					*b;
	ngx_redis_connection_t		*rc;
	struct sockaddr_in			address;

	rc = cycle->redis;

	if (rc == NULL) {
		rc = ngx_pcalloc(cycle->pool, sizeof(ngx_redis_connection_t));
		if (rc == NULL) {
			ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
					"ngx_pcalloc failed(in ngx_redis_init_connection).");
			return NGX_ERROR;
		}
	}

	b = rc->buffer;

	if (b == NULL) {
		b = ngx_create_temp_buf(cycle->pool, NGX_REDIS_BUFFER_SIZE);
		if (b == NULL) {
			ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
					"ngx_create_temp_buf failed(in ngx_redis_init_connection).");
			return NGX_ERROR;
		}

		rc->buffer = b;
	} 

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == (ngx_socket_t) -1) {
		ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
					  "run socket() failed(in ngx_redis_init_connection).");
		return NGX_ERROR;
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(ngx_redis_ip);
	address.sin_port = htons(ngx_redis_port);

	if (connect(s, (struct sockaddr *)&address, sizeof(address)) == -1) {
		ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
					  "run connect() failed(in ngx_redis_init_connection).");

		if (close(s) == -1) {
			ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
					"run close() failed(in ngx_redis_init_connection).");
		}

		return NGX_ERROR;
	}
	

	rc->fd = s;
	rc->log = cycle->log;	

	cycle->redis = rc;

	return NGX_OK;
}

ngx_int_t
ngx_redis_subscribe(ngx_redis_connection_t *rc)
{
	ssize_t		n;
	char		*command = "*2\r\n$9\r\nsubscribe\r\n$14\r\nqmxj_channel_1\r\n";	

	n = send(rc->fd, command, strlen(command), 0); 
	if (n == -1) {
		ngx_log_error(NGX_LOG_ALERT, rc->log, ngx_errno,
				"run send() failed(in ngx_redis_subscribe).");
		
		if (close(rc->fd) == -1) {
			ngx_log_error(NGX_LOG_ALERT, rc->log, ngx_errno,
					"run close() failed(in ngx_redis_subscribe).");
		}

		return NGX_ERROR;
	}

	return NGX_OK;
}

ssize_t
ngx_redis_read_message(ngx_redis_connection_t *rc) {
	ssize_t		n;

	n = recv(rc->fd, rc->buffer->last, rc->buffer->end - rc->buffer->last, 0);

	ngx_log_error(NGX_LOG_INFO, rc->log, 0, 
				  "recv: fd:%d %z of %uz", rc->fd, n, rc->buffer->end - rc->buffer->last);

	if (n == 0) {
		rc->close = 1;
		return 0;
	}

	if (n > 0) {
		rc->buffer->last += n;
		return n;
	}

    ngx_log_error(NGX_LOG_ALERT, rc->log, ngx_errno,
                  "run recv() failed(in ngx_redis_read_message).");

    return NGX_ERROR;
}

ngx_int_t
ngx_process_parse_message(ngx_redis_connection_t *rc, ngx_buf_t *b) 
{
	u_char	ch, *p;
	
	enum {
		sw_start = 0,
		sw_subscribe,
		sw_subscribe_channel_length,
		sw_subscribe_channel_name,
		sw_subscribe_channel_clinet,
		sw_message,
		sw_message_length,
		sw_message_value,
        //sw_message_too_long,
		sw_error,
	} state;		

	state = rc->state;

	for (p = rc->buffer->pos; p < rc->buffer->last; p++) {

		ch = *p;
		rc->parse_done = 0;

		switch (state) {
			case sw_start:
				switch (ch) {
					case '-' :
						rc->error = 1;
						rc->error_start = p + 1;
						state = sw_error;
						break;

					case '*' :
						state = sw_subscribe;
						break;

					default :
						return NGX_REDIS_PARSE_INVALID_MESSAGE;
				}
				break;

			case sw_error: 
				if (ch == '\r') {
					rc->after_cr = 1;
					break;
				}

				if (ch == '\n' && rc->after_cr) {
                    rc->after_cr = 0;
					rc->error_end = p;
                    *(rc->error_end) = '\0';
					goto done;
				}

                rc->after_cr = 0;
                break;

			case sw_subscribe:
				
				if (ngx_strncmp(p, "3\r\n$9\r\nsubscribe\r\n$", strlen("3\r\n$9\r\nsubscribe\r\n$"))
					!= 0)
				{
					return NGX_REDIS_PARSE_INVALID_SUBSCRIBE;
				}

                rc->number_index = 0;
				p = p + strlen("3\r\n$9\r\nsubscribe\r\n$") - 1;
				state = sw_subscribe_channel_length;
				break;

			case sw_subscribe_channel_length:
				if (ch == '\r') {
                    rc->after_cr = 1;
					continue;
				}	

				if (ch == '\n' && rc->after_cr) {
                    rc->after_cr = 0;
                    rc->number[rc->number_index] = '\0';
					rc->number_length = atoi(rc->number);

					if (rc->number_length <= 0 || rc->number_length > 20) {
						return NGX_REDIS_PARSE_INVALID_CHANNEL_NAME;
					}

                    rc->channel_name_length = rc->number_length;
                    rc->channel_name_index = 0;
					state = sw_subscribe_channel_name;
					break;
				}

                rc->after_cr = 0;
				if (ch >= '0' && ch <= '9') {
                    rc->number[rc->number_index] = ch;
                    rc->number_index++;
					break;
				}

				return NGX_REDIS_PARSE_INVALID_LENGTH;

			case sw_subscribe_channel_name:

				if (ch == '\r') {
                    rc->after_cr = 1;
					continue;
				}

				if (ch == '\n' && rc->after_cr) {
                    rc->after_cr = 0;
					rc->channel_name[rc->channel_name_index] = '\0';

                    if ((size_t) rc->channel_name_length != strlen(rc->channel_name)) {
                        return NGX_REDIS_PARSE_INVALID_CHANNEL_NAME;
                    }

					sprintf(rc->channel_message_prefix, "*3\r\n$7\r\nmessage\r\n$%d\r\n%s\r\n$",
							rc->channel_name_length, rc->channel_name);
					rc->channel_message_prefix_n = strlen(rc->channel_message_prefix);

					state = sw_subscribe_channel_clinet;
					break;
				}

                rc->after_cr = 0;
				if (rc->channel_name_index >= (ngx_uint_t) rc->channel_name_length) {
					return NGX_REDIS_PARSE_INVALID_CHANNEL_NAME;
                }

                rc->channel_name[rc->channel_name_index] = ch;
                rc->channel_name_index++;
                break;

			case sw_subscribe_channel_clinet:
			
				if (ch == ':') {
					continue;
				}

				if (ch == '\r') {
                    rc->after_cr = 1;
					continue;
				}

				if (ch == '\n' && rc->after_cr) {
					// pass
					// redis 返回的已有多少个client订阅
                    rc->after_cr = 0;
					state = sw_message;
					break;
				}

                rc->after_cr = 0;
				if (ch >= '0' && ch <= '9') {
                    // pass
                    break;
				}

				return NGX_REDIS_PARSE_INVALID_MESSAGE;

			case sw_message:

				if (ngx_strncmp(p, rc->channel_message_prefix, rc->channel_message_prefix_n)
					 != 0)
				{
					return NGX_REDIS_PARSE_INVALID_MESSAGE;
				}	

                p = p + rc->channel_message_prefix_n - 1;
                rc->number_index = 0;
				state = sw_message_length;
				break;

			case sw_message_length:

				if (ch == '\r') {
					rc->after_cr = 1;
					continue;
				}	

				if (ch == '\n' && rc->after_cr) {
					rc->number[rc->number_index] = '\0';
					rc->message_length = atoi(rc->number);

					if (rc->message_length > 256) {
						//state = sw_message_too_long;
                        //
                        return NGX_REDIS_PARSE_INVALID_MESSAGE;
						break;
					}

					rc->message_start = p + 1;
					state = sw_message_value;
					break;
				}

                rc->after_cr = 0;
				if (ch >= '0' && ch <= '9') {
					rc->number[rc->number_index] = ch;
					rc->number_index++;
					break;
				}

				return NGX_REDIS_PARSE_INVALID_LENGTH;

			case sw_message_value:

				if (rc->message_length) {
					rc->message_length--;
					break;
				}

				if (ch == '\r') {
                    rc->after_cr = 1;
					continue;
				}

				if (ch == '\n' && rc->after_cr) {
                    rc->after_cr = 0;
                    rc->message_end = p - 1;
					state = sw_message;
					goto done;
				}

                rc->after_cr = 0;
				break;
		}
	}
	
    b->pos = p;
	rc->state = state;
	return NGX_AGAIN;

done:
	
    b->pos = p + 1;
	rc->state = sw_message;
	return NGX_OK;
}
