#include <ngx_config.h>
#include <ngx_core.h>
#include <nginx.h>


#if 0
static void ngx_show_version_info(void);
static ngx_int_t ngx_get_options(int argc, char *const *argv);
static ngx_int_t ngx_save_argv(ngx_cycle_t *cycle, int argc, char *const *argv);

static ngx_uint_t       ngx_show_help;
static ngx_uint_t       ngx_show_version;
static u_char           *ngx_prefix;
    
static char **ngx_os_environ;

int
main(int argc, char *const *argv)
{
    ngx_log_t           *log;
    ngx_cycle_t         *cycle, init_cycle; 

    // 初始化 errno 对应的错误信息，用于日志打印
    // os/unix/ngx_errno.c
    if (ngx_strerror_init() != NGX_OK) {
        return 1;
    }

    // 解析命令行参数
    if (ngx_get_options(argc, argv) != NGX_OK) {
        return 1;
    }

    if (ngx_show_version) {
        ngx_show_version_info();

        return 0;
    }

    // 时间初始化变量
    // core/ngx_times.c
    ngx_time_init();

    ngx_pid = getpid();
    ngx_parent = getppid();

    // 日志初始化, 指向 prefix /logs/error.log 或者 标准错误
    // ngx_prefix 为命令行配置
    log = ngx_log_init(ngx_prefix);
    if (log == NULL) {
        return 1;
    }

    ngx_memzero(&init_cycle, sizeof(ngx_cycle_t));
    init_cycle.log = log;
    ngx_cycle = &init_cycle;

    // 内存池分配
    init_cycle.pool = ngx_create_pool(1024, log);
    if (init_cycle.pool == NULL) {
        return 1;
    }

    // 保存argv, argc
    if (ngx_save_argv(&init_cycle, argc, argv) != NGX_OK) {
        return 1;
    }

#if 0
    // 配置各种路径
    if (ngx_process_options(&init_cycle) != NGX_OK) {
        return 1;
    }
#endif

    // 系统配置相关, pagesize, cacheline_size...
    //if (ngx_os_init(log) != NGX_OK) {
    if (1) {
        if (ngx_init_setproctitle(log) != NGX_OK) {
            return 1;
        }
        ngx_time_t  *tp;
        tp = ngx_timeofday();
        srandom(((unsigned) ngx_pid << 16) ^ tp->sec ^ tp->msec);
    }

#if 0
    if (ngx_add_inherited_sockets(&init_cycle) != NGX_OK) {
        return 1;
    }
#endif

    cycle = ngx_init_cycle(&init_cycle);
    if (cycle == NULL) {
        ngx_log_stderr(0, "cycle init failed.");
        return 1;
    }

    ngx_cycle = cycle;

    if (ngx_init_signals(cycle->log) != NGX_OK) {
        return 1;
    }

    if (ngx_daemon(cycle->log) != NGX_OK) {
        return 1;
    }

    ngx_daemonized = 1;

    ngx_str_t pid_file = ngx_string(NGX_PREFIX NGX_ERROR_PID_FILE_PATH);
    if (ngx_create_pidfile(&pid_file, cycle->log) != NGX_OK) {
        return 1;
    }

    /*
    if (ngx_log_redirect_stderr(cycle) != NGX_OK) {
        return 1;
    }

    if (log->file->fd != ngx_stderr) {
        if (ngx_close_file(log->file->fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_ALERT, cycle->log, ngx_errno,
                          ngx_close_file_n " built-in log failed");
        }
    }
    */

    ngx_use_stderr = 0;

    ngx_master_process_cycle(cycle);

    return 0;
}

static void
ngx_show_version_info(void)
{
    ngx_write_stderr("nginx version: " NGINX_VERSION NGX_LINEFEED);

    if (ngx_show_help) {
        ngx_write_stderr(
            "Usage: nginx [-?hvV]" NGX_LINEFEED
                         NGX_LINEFEED
            "Options:" NGX_LINEFEED
            "  -?,-h         : this help" NGX_LINEFEED
            "  -v            : show version and exit" NGX_LINEFEED
            "  -V            : show version and configure options then exit"
                               NGX_LINEFEED
        );
    }
}

static ngx_int_t
ngx_get_options(int argc, char *const *argv)
{
    u_char          *p;
    ngx_int_t       i;

    for (i = 1; i < argc; i++) {
        p = (u_char *) argv[i];

        if (*p++ != '-') {
            ngx_log_stderr(0, "invalid option: \"%s\"", argv[i]);
            return NGX_ERROR;
        }

        while (*p) {
            switch (*p++) {
                case '?':
                case 'h':
                    ngx_show_version = 1;
                    ngx_show_help = 1;
                    break;

                case 'v':
                case 'V':
                    ngx_show_version = 1;
                    break;

                default:
                    ngx_log_stderr(0, "invalid option: \"%c\"", *(p-1));
                    return NGX_ERROR;
            }
        }
    }

    return NGX_OK;
}

static ngx_int_t
ngx_save_argv(ngx_cycle_t *cycle, int argc, char *const *argv)
{
    size_t     len;
    ngx_int_t  i;

    ngx_os_argv = (char **) argv;
    ngx_argc = argc;

    ngx_argv = ngx_alloc((argc + 1) * sizeof(char *), cycle->log);
    if (ngx_argv == NULL) {
        return NGX_ERROR;
    }

    for (i = 0; i < argc; i++) {
        len = ngx_strlen(argv[i]) + 1;

        ngx_argv[i] = ngx_alloc(len, cycle->log);
        if (ngx_argv[i] == NULL) {
            return NGX_ERROR;
        }

        (void) ngx_cpystrn((u_char *) ngx_argv[i], (u_char *) argv[i], len);
    }

    ngx_argv[i] = NULL;

    ngx_os_environ = environ;

    return NGX_OK;
}

#endif

