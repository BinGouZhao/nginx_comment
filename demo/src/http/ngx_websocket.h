#ifndef _NGX_WEBSOCKET_H_INCLUDE_
#define _NGX_WEBSOCKET_H_INCLUDE_

#include <ngx_config.h>
#include <ngx_core.h>

#define WEBSOCKET_FRAME_SET_FIN(BYTE) (((BYTE) & 0x01) << 7)
#define WEBSOCKET_FRAME_SET_OPCODE(BYTE) ((BYTE) & 0x0F)
#define WEBSOCKET_FRAME_SET_MASK(BYTE) (((BYTE) & 0x01) << 7)
#define WEBSOCKET_FRAME_SET_LENGTH(X64, IDX) (unsigned char)(((X64) >> ((IDX)*8)) & 0xFF)

typedef enum {  
    NGX_WS_CONNECTION_WRITE_STATE = 0,
    NGX_WS_CONNECTION_READ_STATE,
    NGX_WS_CONNECTION_PING_STATE,
    NGX_WS_CONNECTION_PONG_STATE,
    NGX_WS_CONNECTION_CLOSING_STATE,
} ngx_ws_state_e;

typedef struct ngx_websocket_frame_s ngx_websocket_frame_t;
typedef struct ngx_websocket_connection_s ngx_websocket_connection_t;

struct ngx_websocket_frame_s {
    void            *data;

    ngx_int_t       message_type;

    ngx_buf_t       message;

    time_t          start_sec;
    ngx_msec_t      start_msec;
};

struct ngx_websocket_connection_s {
    void                    *data;
    ngx_int_t               state;
    ngx_queue_t             queue;

    ngx_pool_t              *pool;

    u_char                  buffer[NGX_WEBSOCKET_MAX_MESSAGE_LENGTH];
    size_t                  message_size; 

    ngx_uint_t              channel_id;
    ngx_uint_t              message_id;

	unsigned				in_queue:1;
};

struct ngx_websocket_channel_s {
    ngx_uint_t      channel_id;

    ngx_queue_t     clients;
    ngx_uint_t      clients_n;

    ngx_pool_t      *pool;
//    ngx_chain_t     messages;

    ngx_log_t       *log;
};


ngx_int_t ngx_websocket_init();
void ngx_websocket_init_connection(ngx_connection_t *c, ngx_uint_t channel_id);
ngx_uint_t ngx_websocket_frame_encode(u_char *buffer, u_char *message, char opcode, uint8_t finish, ngx_uint_t length);
void ngx_websocket_process_messages(ngx_cycle_t *cycle);

#endif
