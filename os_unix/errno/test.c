#include "ngx_errno.h"
#include "ngx_string.h"

void main() {
    if (ngx_strerror_init() != NGX_OK) {
        printf("error list init failed.\n");
        return;
    }

    int i; 
    u_char *errstr;

    errstr = malloc(sizeof(char) * 40);

    //for (i = 0; i < NGX_SYS_NERR; i++) {
    for (i = 0; i < 10; i++) {
        ngx_strerror(i, errstr, 40);
        printf("%s.\n", errstr);
    }
}
