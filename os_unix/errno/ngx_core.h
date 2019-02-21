#ifndef _NGX_CORE_H_INCLUDED_
#define _NGX_CORE_H_INCLUDED_


#include "ngx_config.h"
#include "ngx_string.h"
#include "ngx_errno.h"

#define  NGX_OK          0
#define  NGX_ERROR      -1
#define  NGX_AGAIN      -2
#define  NGX_BUSY       -3
#define  NGX_DONE       -4
#define  NGX_DECLINED   -5
#define  NGX_ABORT      -6

#define LF     (u_char) '\n'

#define ngx_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))

#endif /* _NGX_CORE_H_INCLUDED_ */
