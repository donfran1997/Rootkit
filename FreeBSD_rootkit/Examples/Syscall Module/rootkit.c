#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/sysent.h>
#include <sys/sysproto.h>

/*
    Demonstrating registering a syscall module.
*/
typedef struct _sys_call_args{
    char *str_;
} sys_args;

// actual syscall function
static int print_str(struct thread *td, void *args)
{
    sys_args *s_args = (sys_args *) args;

    printf("You sent me: %s\n", s_args->str_);
    return 0;
}

static struct sysent my_sysent = {
    1,          // # args
    print_str   // name of syscall function
};

static int offset = NO_SYSCALL;

/*
    Routine called when rookit gets loaded/unloaded.
*/
static int load_handler(struct module *module, int cmd, void *arg)
{
    int error = 0;

    switch(cmd){
        case MOD_LOAD:
            uprintf("Rootkit has been loaded!\n");
            break;

        case MOD_UNLOAD:
            uprintf("Rootkit has been unloaded!\n");
            break;

        default:
            error = EOPNOTSUPP;
            break;
    }

    return error;
}

static moduledata_t handler_mod = {
    "Not a rootkit!",
    load_handler,
    NULL
};

// DECLARE_MODULE(rkit, handler_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
SYSCALL_MODULE(print_str, &offset, &my_sysent, load_handler, NULL);
