#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#define ngx_str3_cmp(m, c0, c1, c2, c3)                                       \
    *(uint32_t *) m == ((c3 << 24) | (c2 << 16) | (c1 << 8) | c0)

ngx_int_t
ngx_http_parse_request_line(ngx_http_request_t *r, ngx_buf_t *b)
{
	u_char	ch, *p, *m; //c
	enum {
		sw_start = 0,
		sw_method,
		sw_spaces_before_uri,
		sw_after_slash_in_uri,
		sw_check_uri_http_09,
		sw_http_H,
		sw_http_HT,
		sw_http_HTT,
		sw_http_HTTP,
		sw_first_major_digit,
		sw_major_digit,
		sw_first_minor_digit,
		sw_minor_digit,
		sw_spaces_after_digit,
		sw_almost_done
	} state;

	state = r->state;

	for (p = b->pos; p < b->last; p++) {
		ch = *p;

		switch (state) {

			case sw_start:
				r->request_start = p;

				if (ch == CR || ch == LF) {
					break;
				}

				// websocket 只支持 get
				if (ch != 'G') {
					return NGX_HTTP_PARSE_INVALID_METHOD;
				}

				state = sw_method;
				break;


			case sw_method:
				m = r->request_start;

				if (ngx_str3_cmp(m, 'G', 'E', 'T', ' ')) {
					r->method_end = p + 2;
					r->method = NGX_HTTP_GET;
				} else {
					return NGX_HTTP_PARSE_INVALID_METHOD;
				}

				p = p + 1;
				state = sw_spaces_before_uri;
				break;

			case sw_spaces_before_uri:

				if (ch == '/') {
					r->uri_start = p;
					state = sw_after_slash_in_uri;
					break;
				}

				switch (ch) {
					case ' ':
						break;
					default:
						return NGX_HTTP_PARSE_INVALID_REQUEST;
				}
				break;

			case sw_after_slash_in_uri:

				switch (ch) {
					case ' ':
						r->uri_end = p;
						state = sw_check_uri_http_09;
						break;
					case CR:
						r->uri_end = p;
						r->http_minor = 9;
						state = sw_almost_done;
						break;
					case LF:
						r->uri_end = p;
						r->http_minor = 9;
						goto done;
					case '\0':
						return NGX_HTTP_PARSE_INVALID_REQUEST;
					default:
						break;
				}
				break;

			case sw_check_uri_http_09:
				switch (ch) {
					case ' ':
						break;
					case CR:
						r->http_minor = 9;
						state = sw_almost_done;
						break;
					case LF:
						r->http_minor = 9;
						goto done;
					case 'H':
						r->http_protocol.data = p;
						state = sw_http_H;
						break;
					default:
						return NGX_HTTP_PARSE_INVALID_REQUEST;
				}
				break;

			case sw_http_H:
				switch (ch) {
					case 'T':
						state = sw_http_HT;
						break;
					default:
						return NGX_HTTP_PARSE_INVALID_REQUEST;
				}
				break;

			case sw_http_HT:
				switch (ch) {
					case 'T':
						state = sw_http_HTT;
						break;
					default:
						return NGX_HTTP_PARSE_INVALID_REQUEST;
				}
				break;

			case sw_http_HTT:
				switch (ch) {
					case 'P':
						state = sw_http_HTTP;
						break;
					default:
						return NGX_HTTP_PARSE_INVALID_REQUEST;
				}
				break;

			case sw_http_HTTP:
				switch (ch) {
					case '/':
						state = sw_first_major_digit;
						break;
					default:
						return NGX_HTTP_PARSE_INVALID_REQUEST;
				}
				break;

				/* first digit of major HTTP version */
			case sw_first_major_digit:
				if (ch < '1' || ch > '9') {
					return NGX_HTTP_PARSE_INVALID_REQUEST;
				}

				r->http_major = ch - '0';

				if (r->http_major > 1) {
					return NGX_HTTP_PARSE_INVALID_VERSION;
				}

				state = sw_major_digit;
				break;

				/* major HTTP version or dot */
			case sw_major_digit:
				if (ch == '.') {
					state = sw_first_minor_digit;
					break;
				}

				if (ch < '0' || ch > '9') {
					return NGX_HTTP_PARSE_INVALID_REQUEST;
				}

				r->http_major = r->http_major * 10 + (ch - '0');

				if (r->http_major > 1) {
					return NGX_HTTP_PARSE_INVALID_VERSION;
				}

				break;

			case sw_first_minor_digit:
				if (ch < '0' || ch > '9') {
					return NGX_HTTP_PARSE_INVALID_REQUEST;
				}

				r->http_minor = ch - '0';
				state = sw_minor_digit;
				break;

				/* minor HTTP version or end of request line */
			case sw_minor_digit:
				if (ch == CR) {
					state = sw_almost_done;
					break;
				}

				if (ch == LF) {
					goto done;
				}

				if (ch == ' ') {
					state = sw_spaces_after_digit;
					break;
				}

				if (ch < '0' || ch > '9') {
					return NGX_HTTP_PARSE_INVALID_REQUEST;
				}

				if (r->http_minor > 99) {
					return NGX_HTTP_PARSE_INVALID_REQUEST;
				}

				r->http_minor = r->http_minor * 10 + (ch - '0');
				break;

			case sw_spaces_after_digit:
				switch (ch) {
					case ' ':
						break;
					case CR:
						state = sw_almost_done;
						break;
					case LF:
						goto done;
					default:
						return NGX_HTTP_PARSE_INVALID_REQUEST;
				}
				break;

				/* end of request line */
			case sw_almost_done:
				r->request_end = p - 1;
				switch (ch) {
					case LF:
						goto done;
					default:
						return NGX_HTTP_PARSE_INVALID_REQUEST;
				}
		}
	}


	b->pos = p;
	r->state = state;

	return NGX_AGAIN;

done:

	b->pos = p + 1;

	if (r->request_end == NULL) {
		r->request_end = p;
	}

	r->http_version = r->http_major * 1000 + r->http_minor;
	r->state = sw_start;

	// HTTP/1.1 and GET
	if (r->http_version < NGX_HTTP_VERSION_11 || r->method != NGX_HTTP_GET) {
		return NGX_HTTP_PARSE_INVALID_VERSION;
	}

	return NGX_OK;
}

ngx_int_t
ngx_http_parse_header_line(ngx_http_request_t *r, ngx_buf_t *b,
	ngx_uint_t	allow_underscores)
{
	u_char			c, ch, *p;
	ngx_uint_t		hash, i;
	enum {
		sw_start = 0,
        sw_name,
        sw_space_before_value,
        sw_value,
        sw_space_after_value,
        sw_ignore_line,
        sw_almost_done,
        sw_header_almost_done
    } state;

    static u_char  lowcase[] =
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0-\0\0" "0123456789\0\0\0\0\0\0"
        "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
        "\0abcdefghijklmnopqrstuvwxyz\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
        "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

	state = r->state;
	hash = r->header_hash;
	i = r->lowcase_index;

	for (p = b->pos; p < b->last; p++) {
		ch = *p;

		switch (state) {

			/* first char */
			case sw_start:
				r->header_name_start = p;
				r->invalid_header = 0;

				switch (ch) {
					case CR:
						r->header_end = p;
						state = sw_header_almost_done;
						break;
					case LF:
						r->header_end = p;
						goto header_done;
					default:
						state = sw_name;

						c = lowcase[ch];

						if (c) {
							hash = ngx_hash(0, c);
							r->lowcase_header[0] = c;
							i = 1;
							break;
						}

						if (ch == '_') {
							if (allow_underscores) {
								hash = ngx_hash(0, ch);
								r->lowcase_header[0] = ch;
								i = 1;

							} else {
								r->invalid_header = 1;
							}

							break;
						}

						if (ch == '\0') {
							return NGX_HTTP_PARSE_INVALID_HEADER;
						}

						r->invalid_header = 1;

						break;

				}
				break;

				/* header name */
			case sw_name:
				c = lowcase[ch];

				if (c) {
					hash = ngx_hash(hash, c);
					r->lowcase_header[i++] = c;
					i &= (NGX_HTTP_LC_HEADER_LEN - 1);
					break;
				}

				if (ch == '_') {
					if (allow_underscores) {
						hash = ngx_hash(hash, ch);
						r->lowcase_header[i++] = ch;
						i &= (NGX_HTTP_LC_HEADER_LEN - 1);

					} else {
						r->invalid_header = 1;
					}

					break;
				}

				if (ch == ':') {
					r->header_name_end = p;
					state = sw_space_before_value;
					break;
				}

				if (ch == CR) {
					r->header_name_end = p;
					r->header_start = p;
					r->header_end = p;
					state = sw_almost_done;
					break;
				}

				if (ch == LF) {
					r->header_name_end = p;
					r->header_start = p;
					r->header_end = p;
					goto done;
				}
#if 0
				/* IIS may send the duplicate "HTTP/1.1 ..." lines */
				if (ch == '/'
						&& r->upstream
						&& p - r->header_name_start == 4
						&& ngx_strncmp(r->header_name_start, "HTTP", 4) == 0)
				{
					state = sw_ignore_line;
					break;
				}
#endif

				if (ch == '\0') {
					return NGX_HTTP_PARSE_INVALID_HEADER;
				}

				r->invalid_header = 1;

				break;

				/* space* before header value */
			case sw_space_before_value:
				switch (ch) {
					case ' ':
						break;
					case CR:
						r->header_start = p;
						r->header_end = p;
						state = sw_almost_done;
						break;
					case LF:
						r->header_start = p;
						r->header_end = p;
						goto done;
					case '\0':
						return NGX_HTTP_PARSE_INVALID_HEADER;
					default:
						r->header_start = p;
						state = sw_value;
						break;
				}
				break;

				/* header value */
			case sw_value:
				switch (ch) {
					case ' ':
						r->header_end = p;
						state = sw_space_after_value;
						break;
					case CR:
						r->header_end = p;
						state = sw_almost_done;
						break;
					case LF:
						r->header_end = p;
						goto done;
					case '\0':
						return NGX_HTTP_PARSE_INVALID_HEADER;
				}
				break;

				/* space* before end of header line */
			case sw_space_after_value:
				switch (ch) {
					case ' ':
						break;
					case CR:
						state = sw_almost_done;
						break;
					case LF:
						goto done;
					case '\0':
						return NGX_HTTP_PARSE_INVALID_HEADER;
					default:
						state = sw_value;
						break;
				}
				break;

				/* ignore header line */
			case sw_ignore_line:
				switch (ch) {
					case LF:
						state = sw_start;
						break;
					default:
						break;
				}
				break;

				/* end of header line */
			case sw_almost_done:
				switch (ch) {
					case LF:
						goto done;
					case CR:
						break;
					default:
						return NGX_HTTP_PARSE_INVALID_HEADER;
				}
				break;

				/* end of header */
			case sw_header_almost_done:
				switch (ch) {
					case LF:
						goto header_done;
					default:
						return NGX_HTTP_PARSE_INVALID_HEADER;
				}
		}
	}

	b->pos = p;
	r->state = state;
	r->header_hash = hash;
	r->lowcase_index = i;

	return NGX_AGAIN;

done:

	b->pos = p + 1;
	r->state = sw_start;
	r->header_hash = hash;
	r->lowcase_index = i;

	return NGX_OK;

header_done:

	b->pos = p + 1;
	r->state = sw_start;

	return NGX_HTTP_PARSE_HEADER_DONE;
}
