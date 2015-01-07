#include "infer.h"
#include <null.h>

static void clear_argument(struct argument *arg)
{
	if (!arg) return;
        arg->type = ARG_NONE;
}

static void clear_request(struct request *req)
{
	if (!req) return;
        req->command = CMD_NONE;
        req->num_args = 0;
        clear_argument(&req->arg1);
        clear_argument(&req->arg2);
        clear_argument(&req->arg3);
        clear_argument(&req->arg4);
        req->outarg_type = ARG_NONE;
}

static void clear_response(struct response *res)
{
	if (!res) return;
        res->ok = 0;
        res->outarg.type = ARG_NONE;
}


static void send_response(const int fd, struct response *res)
{
	write(fd, res, sizeof(struct response));
}

static void send_error_response(const int fd)
{
        struct response res;
        clear_response(&res);
        send_response(fd, &res);
}

static void handleAlloc(struct request *req, struct response *res)
{
	int i;
	int size = req->arg1.val.num;
	int value = req->arg2.val.num;
	char *newmem = NULL; 

	// allocate memory
	int retval = cgc_allocate(size, 0, &newmem);

	if (retval == 0)
	{
		// initialize to specified value
		for (i = 0; i < size; ++i)
			newmem[i] = value;

		res->ok = 1;
		res->outarg.type = ARG_PTR;
		res->outarg.val.addr = newmem;
	}
	else
	{
		res->ok = 0;
	}
}

/* <bytes> READ <addr> <numBytes> */
static void handleRead(struct request *req, struct response *res)
{
        uintptr_t ptr = req->arg1.val.addr;
        int num_bytes = req->arg2.val.num;
        int i;

        if (num_bytes <= MAX_NUM_BYTES)
                res->ok = 1;
        else
        {
                num_bytes = MAX_NUM_BYTES;
                res->ok = 0;
        }

        res->outarg.type = ARG_BYTES;
        res->outarg.val.bytes.num_bytes = num_bytes;

        for (i = 0; i < num_bytes; ++i)
        {
                char *tmp = ptr;
                res->outarg.val.bytes.bytes[i] = tmp[i];
        }
}

/* WRITE <addr> <bytes> */
static void handleWrite(struct request *req, struct response *res)
{
        uintptr_t ptr = req->arg1.val.addr;
        int num_bytes = req->arg2.val.bytes.num_bytes;
        char *bytes = req->arg2.val.bytes.bytes;
        int i;

//        printf("WRITE %p %d\n", ptr, num_bytes);

        for (i = 0; i < num_bytes; ++i)
                ((char*)ptr)[i] = bytes[i];

        res->ok = 1;
}

static uintptr_t get_func_arg(struct argument *arg)
{
        if (arg->type == ARG_INT)
                return (uintptr_t) arg->val.num;
        else if (arg->type == ARG_PTR)
                return (uintptr_t) arg->val.addr;
        else if (arg->type == ARG_BYTES)
                return (uintptr_t) arg->val.bytes.bytes;
        else
                return (uintptr_t) 0x0;
}

/* outarg CALL <fn*> [arg] [arg] [arg] */
static void handleCall(struct request *req, struct response *res)
{
        struct argument outarg; // void, int, void*

        struct argument *arg1 = &req->arg1;   // int, void*
        struct argument *arg2 = &req->arg2;   // int, void*
        struct argument *arg3 = &req->arg3;   // int, void*
        struct argument *arg4 = &req->arg4;   // int, void*

        uintptr_t retval;
        uintptr_t (*fn)(uintptr_t, ...);
        uintptr_t fa1, fa2, fa3;

        fn = arg1->val.addr;

        fa1 = get_func_arg(arg2); // arg2 of request maps to function argument 1
        fa2 = get_func_arg(arg3); // arg3 of request maps to function argument 2
        fa3 = get_func_arg(arg4); // arg4 of request maps to function argument 3

//        printf("CALL fn:%p #args: %d ret_type: %d\n", fn, req->num_args, req->outarg_type);

        if (req->num_args >= 1 && req->num_args <= 4)
        {
                retval = (*fn)(fa1, fa2, fa3);

                if (req->outarg_type == ARG_INT) {
//                        printf("return value = %d\n", retval);
                        res->ok = 1;
                        res->outarg.type = ARG_INT;
                        res->outarg.val.num = retval;
                        return;
                } else if (req->outarg_type == ARG_PTR) {
                        res->ok = 1;
                        res->outarg.type = ARG_PTR;
                        res->outarg.val.addr = retval;
 //                       printf("return value(addr) = %p\n", retval);
                        return;
                }
        }
}


static int handleCommand(const int fd, struct request *req, struct response *res)
{
	clear_response(res);

	switch(req->command) {
		case CMD_ALLOC:
			handleAlloc(req, res);
			send_response(fd, res);
			return 0;
			break;
		case CMD_CALL:
			handleCall(req, res);
			send_response(fd, res);
			return 0;
			break;
		case CMD_READ:
			handleRead(req, res);
			send_response(fd, res);
			return 0;
			break;
		case CMD_WRITE:
			handleWrite(req, res);
			send_response(fd, res);
			return 0;
   			break;
		case CMD_QUIT:
		default:
			return 1;
			break;
	}

	return 1;
}

void commandLoop()
{
	int fdin = 0;    // stdin
	int fdout = 1;   // stdout
	int done = 0;
	int bytes_read;
	char buf[sizeof(struct request) + 1024];

	struct request req;
	struct response res;

	do {
		clear_request(&req); 
		read(fdin, &req, sizeof(struct request));
		done = handleCommand(fdout, &req, &res);
	} while (!done);
}
