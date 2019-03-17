#ifndef _NGX_HTTP_REQUEST_H_INCLUDED_
#define _NGX_HTTP_REQUEST_H_INCLUDED_

#define NGX_HTTP_LC_HEADER_LEN             32

#define NGX_HTTP_VERSION_9                 9
#define NGX_HTTP_VERSION_10                1000
#define NGX_HTTP_VERSION_11                1001
#define NGX_HTTP_VERSION_20                2000

#define NGX_HTTP_UNKNOWN                   0x0001
#define NGX_HTTP_GET                       0x0002
#define NGX_HTTP_HEAD                      0x0004
#define NGX_HTTP_POST                      0x0008
#define NGX_HTTP_PUT                       0x0010
#define NGX_HTTP_DELETE                    0x0020
#define NGX_HTTP_MKCOL                     0x0040
#define NGX_HTTP_COPY                      0x0080
#define NGX_HTTP_MOVE                      0x0100
#define NGX_HTTP_OPTIONS                   0x0200
#define NGX_HTTP_PROPFIND                  0x0400
#define NGX_HTTP_PROPPATCH                 0x0800
#define NGX_HTTP_LOCK                      0x1000
#define NGX_HTTP_UNLOCK                    0x2000
#define NGX_HTTP_PATCH                     0x4000
#define NGX_HTTP_TRACE                     0x8000


#define NGX_HTTP_PARSE_HEADER_DONE         1

#define NGX_HTTP_CLIENT_ERROR              10
#define NGX_HTTP_PARSE_INVALID_METHOD      10
#define NGX_HTTP_PARSE_INVALID_REQUEST     11
#define NGX_HTTP_PARSE_INVALID_VERSION     12
#define NGX_HTTP_PARSE_INVALID_09_METHOD   13

#define NGX_HTTP_PARSE_INVALID_HEADER      14

#define NGX_HTTP_CONTINUE                  100
#define NGX_HTTP_SWITCHING_PROTOCOLS       101
#define NGX_HTTP_PROCESSING                102

#define NGX_HTTP_OK                        200
#define NGX_HTTP_CREATED                   201
#define NGX_HTTP_ACCEPTED                  202
#define NGX_HTTP_NO_CONTENT                204
#define NGX_HTTP_PARTIAL_CONTENT           206

#define NGX_HTTP_SPECIAL_RESPONSE          300
#define NGX_HTTP_MOVED_PERMANENTLY         301
#define NGX_HTTP_MOVED_TEMPORARILY         302
#define NGX_HTTP_SEE_OTHER                 303
#define NGX_HTTP_NOT_MODIFIED              304
#define NGX_HTTP_TEMPORARY_REDIRECT        307
#define NGX_HTTP_PERMANENT_REDIRECT        308

#define NGX_HTTP_BAD_REQUEST               400
#define NGX_HTTP_UNAUTHORIZED              401
#define NGX_HTTP_FORBIDDEN                 403
#define NGX_HTTP_NOT_FOUND                 404
#define NGX_HTTP_NOT_ALLOWED               405
#define NGX_HTTP_REQUEST_TIME_OUT          408
#define NGX_HTTP_CONFLICT                  409
#define NGX_HTTP_LENGTH_REQUIRED           411
#define NGX_HTTP_PRECONDITION_FAILED       412
#define NGX_HTTP_REQUEST_ENTITY_TOO_LARGE  413
#define NGX_HTTP_REQUEST_URI_TOO_LARGE     414
#define NGX_HTTP_UNSUPPORTED_MEDIA_TYPE    415
#define NGX_HTTP_RANGE_NOT_SATISFIABLE     416
#define NGX_HTTP_MISDIRECTED_REQUEST       421
#define NGX_HTTP_TOO_MANY_REQUESTS         429

#define NGX_HTTP_CLIENT_CLOSED_REQUEST     499

#define NGX_HTTP_INTERNAL_SERVER_ERROR     500
#define NGX_HTTP_NOT_IMPLEMENTED           501
#define NGX_HTTP_BAD_GATEWAY               502
#define NGX_HTTP_SERVICE_UNAVAILABLE       503
#define NGX_HTTP_GATEWAY_TIME_OUT          504
#define NGX_HTTP_VERSION_NOT_SUPPORTED     505
#define NGX_HTTP_INSUFFICIENT_STORAGE      507




#define NGX_WS_VERSION			13

#define NGX_WS_MASK_KEY_LENGTH	4

#define WEBSOCKET_UUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#define NGX_CLIENT_HEADER_BUFFER_SIZE	1024
#define NGX_POST_ACCEPT_TIMEOUT         100
#define NGX_REQUEST_POOL_SIZE           2018
#define NGX_CLIENT_HEADER_TIMEOUT       100
#define NGX_SEND_RESPONSE_TIMEOUT       100


#define NGX_WS_OPCODE_CONTINUATION		0x0
#define NGX_WS_OPCODE_TEXT				0x1
#define NGX_WS_OPCODE_BINARY			0x2
#define NGX_WS_OPCODE_CLOSE				0x8
#define NGX_WS_OPCODE_PING				0x9
#define NGX_WS_OPCODE_PONG				0xa

#define NGX_WS_CLOSE_NORMAL				1000
#define NGX_WS_CLOSE_GOING_AWAY			1001
#define NGX_WS_CLOSE_PROTOCOL_ERROR		1002
#define NGX_WS_CLOSE_DATA_ERROR			1003
#define NGX_WS_CLOSE_STATUS_ERROR		1005
#define NGX_WS_CLOSE_ABNORMAL			1006
#define NGX_WS_CLOSE_MESSAGE_ERROR		1007
#define NGX_WS_CLOSE_POLICY_ERROR		1008
#define NGX_WS_CLOSE_MESSAGE_TOO_BIG	1009
#define NGX_WS_CLOSE_EXTENSION_MISSING  1010
#define NGX_WS_CLOSE_SERVER_ERROR		1011
#define NGX_WS_CLOSE_TLS				1015

typedef enum {
    NGX_HTTP_INITING_REQUEST_STATE = 0,
    NGX_HTTP_READING_REQUEST_STATE,
    NGX_HTTP_PROCESS_REQUEST_STATE,

    NGX_HTTP_CONNECT_UPSTREAM_STATE,
    NGX_HTTP_WRITING_UPSTREAM_STATE,
    NGX_HTTP_READING_UPSTREAM_STATE,

    NGX_HTTP_WRITING_REQUEST_STATE,
    NGX_HTTP_LINGERING_CLOSE_STATE,
    NGX_HTTP_KEEPALIVE_STATE
} ngx_http_state_e;

typedef enum {
	NGX_WS_INITING_REQUEST_STATE = 0,
	NGX_WS_READING_REQUEST_STATE,
	NGX_WS_PROCESS_REQUEST_STATE,
	
	NGX_WS_KEEPALIVE_CONNECTION_STATE,
	NGX_WS_READ_CONNECTION_STATE,
	NGX_WS_WRITE_CONNECTION_STATE,
	NGX_WS_HANDSHAKE_CONNECTION_STATE,
	NGX_WS_CLOSING_CONNECTION_STATE,
} ngx_ws_state_e;

struct ngx_ws_frame_s {
	unsigned			fin:1;
	unsigned			rsv1:1;
	unsigned			rsv2:1;
	unsigned			rsv3:1;
	unsigned			opcode:4;
	unsigned			mask:1;
	unsigned			length:7;

	uint16_t			header_length;	
	size_t				payload_length;

	char				mask_key[NGX_WS_MASK_KEY_LENGTH];

	ngx_buf_t			*buf;
};

struct ngx_ws_connection_s {
	ngx_connection_t		*connection;

	ngx_pool_t				*pool;

	ngx_buf_t				*header_in;
};

typedef struct {
    int         state;
} ngx_http_connection_t;

typedef struct {
    ngx_list_t      headers;
} ngx_http_headers_in_t;

struct ngx_http_request_s {
    ngx_pool_t      *pool;
    ngx_buf_t       *header_in;
    ngx_buf_t       *buffer;

    time_t          start_sec;
    ngx_msec_t      start_msec;

    ngx_http_connection_t            *http_connection;
    ngx_connection_t                 *connection;
    ngx_http_headers_in_t             headers_in;

	ngx_uint_t		method;
	ngx_uint_t      http_version;

    ngx_str_t       request_line;
    off_t           request_length;

    unsigned        count:16;
    unsigned        http_state:4;
    unsigned        invalid_header:1;
    unsigned        upgrade:1;
    unsigned        error:1;

    u_char          *sec_websocket_key;

	ngx_uint_t		state;
    //ngx_uint_t      sent;


    ngx_uint_t      header_hash;
    ngx_uint_t      lowcase_index;
    u_char          lowcase_header[NGX_HTTP_LC_HEADER_LEN];

	ngx_str_t       http_protocol;

    u_char          *header_name_start;
    u_char          *header_name_end;
    u_char			*header_start;
    u_char          *header_end;

	u_char			*request_start;
	u_char			*request_end;
	u_char			*method_end;
	u_char			*uri_start;

	u_char			*uri_end;

	unsigned		http_minor:16;
	unsigned        http_major:16;
};
#endif
