#ifndef _NGX_WEBSOCKET_H_INCLUDE_
#define _NGX_WEBSOCKET_H_INCLUDE_

#include <ngx_config.h>
#include <ngx_core.h>

#define WEBSOCKET_FRAME_SET_FIN(BYTE) (((BYTE) & 0x01) << 7)
#define WEBSOCKET_FRAME_SET_OPCODE(BYTE) ((BYTE) & 0x0F)
#define WEBSOCKET_FRAME_SET_MASK(BYTE) (((BYTE) & 0x01) << 7)
#define WEBSOCKET_FRAME_SET_LENGTH(X64, IDX) (unsigned char)(((X64) >> ((IDX)*8)) & 0xFF)


typedef struct ngx_websocket_frame_s ngx_websocket_frame_t;

struct ngx_websocket_frame_s {
    void            *data;

    ngx_int_t       message_type;

    ngx_buf_t       message;

    time_t          start_sec;
    ngx_msec_t      start_msec;
};

struct ngx_websocket_connection_s {
    ngx_int_t               state;
    ngx_queue_t             queue;

    ngx_pool_t              *pool;

    u_char                  buffer[NGX_WEBSOCKET_MAX_MESSAGE_LENGTH];
    ngx_uint_t              sent; 
};

struct ngx_websocket_channel_s {
    ngx_uint_t      channel_id;

    ngx_queue_t     clients;
    ngx_uint_t      clients_n;

    ngx_pool_t      *pool;
//    ngx_chain_t     messages;

    ngx_log_t       *log;
};

void ngx_websocket_send(ngx_connection_t *c);
ngx_uint_t ngx_websocket_frame_encode(u_char *buffer, u_char *message, char opcode, uint8_t finish);

#endif
