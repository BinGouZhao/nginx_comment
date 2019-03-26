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

	if (connect(s, struct sockaddr *)&address, sizeof(address) == -1) {
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
	ngx_err_t	err;

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

	err = ngx_socket_errno;
	n = ngx_connection_error(c, err, "recv() failed");

	// NGX_ERROR
	return n;
}

ngx_int_t
ngx_process_parse_message(ngx_redis_connection_t *rc, ngx_buf_t *b) 
{
	int		i, n;
	int		message_length;
	u_char	ch, *p;
	
	enum {
		sw_start = 0,
		sw_subscribe,
		sw_subscribe_channel_length,
		sw_subscribe_channel_name,
		sw_subscribe_channel_clinet_num,
		sw_message,
		sw_message_length,
		sw_message_value,
		sw_error,
		sw_almost_done,
	} state;		

	message_length = 0;
	state = rc->state;

	for (p = rc->buffer->pos; p < rc->buffer->last; p++) {

		ch = *p;

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
					rc->error_end = p;
					goto done;
				}




			case sw_subscribe:
				
				if (ngx_strncmp(p, "\r\n$9\r\nsubscribe\r\n$", strlen("\r\n$9\r\nsubscribe\r\n$"))
					!= 0)
				{
					return NGX_REDIS_PARSE_INVALID_SUBSCRIBE;
				}

				i = 0;
				p = p + strlen("\r\n$9\r\nsubscribe\r\n$") - 1;
				state = sw_subscribe_channel_length;
				break;

			case sw_subscribe_channel_length:
				if (ch == '\r') {
					continue;
				}	

				if (ch == '\n') {
					number[i] = '\0';
					rc->channel_name_length = atoi(number);

					if (rc->channel_name_length <= 0 || rc->channel_name_length > 20) {
						return NGX_REDIS_PARSE_INVALID_CHANNEL_NAME;
					}

					n = 0;
					state = sw_subscribe_channel_name;
					break;
				}

				if (ch >= '0' && ch <= '9') {
					number[i] = ch;
					i++;
					break;
				}

				return NGX_REDIS_PARSE_INVALID_LENGTH;

			case sw_subscribe_channel_name:

				if (ch == '\r') {
					continue;
				}

				if (ch == '\n') {
					rc->channel_name[n] = '\0';
					state = sw_subscribe_channel_clinet_num;
					i = 0;

					sprintf(rc->channel_message_prefix, "*3\r\n$7\r\nmessage\r\n$%d\r\n%s\r\n$",
							rc->channel_name_length, rc->channel_name);
					rc->channel_message_prefix_n = strlen(rc->channel_message_prefix);

					break;
				}

				if (n < rc->channel_name_length) {
					rc->channel_name[n] = ch;
					n++;
				} else {
					return NGX_REDIS_PARSE_INVALID_CHANNEL_NAME;
				}

				break;

			case sw_subscribe_channel_clinet:
			
				if (ch == ';') {
					continue;
				}

				if (ch == '\r') {
					continue;
				}

				if (ch == '\n') {
					// pass
					// redis 返回的已有多少个client订阅
					state = sw_message;
					break;
				}

				if (ch >= '0' && ch <= '9') {
					number[i] = ch;
					i++;

					if (i > 20) {
						return NGX_REDIS_PARSE_INVALID_LENGTH;
					}
					break;
				}

				return NGX_REDIS_PARSE_INVALID_MESSAGE;

			case sw_message:

				if (ngx_strncmp(p, rc->channel_message_prefix, rc->channel_message_prefix_n)
					 != 0)
				{
					return NGX_REDIS_PARSE_INVALID_MESSAGE;
				}	

				state = sw_message_length;
				i = 0;
				break;

			case sw_message_length:

				if (ch == '\r') {
					rc->after_cr = 1;
					continue;
				}	

				if (ch == '\n' && rc->after_cr) {
					rc->number[rc->number_index] = '\0';
					message_length = atoi(rc->number);

					if (message_length > 256) {
						state = sw_message_too_long;
						break;
					}

					if (message_length <= 0) {
						state = sw_message;
						goto done;
					}

					rc->message_start = p + 1;
					rc->message_length = message_length;
					state = sw_subscribe_message_value;
					break;
				}

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
					continue;
				}

				if (ch == '\n') {
					state = sw_message;
					goto done;
				}

				rc->message_end = p;

				break;
		}
	}
	
	rc->state = state;
	return NGX_AGAIN;

done:
	
	rc->state = sw_message;
	return NGX_OK;
}
