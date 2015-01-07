#ifndef _INFER_FN_
#define _INFER_FN_

#include <stdint.h>
#include <syscall.h>

#define MAX_NUM_BYTES 512

/*
 *    void* ALLOC <size> <value>
 *    outarg CALL <fn*> [arg] [arg] [arg]
 *    WRITE <addr> <bytes>
 *    <bytes> READ <addr> <numbytes>
 *
 */
enum command_type { CMD_NONE, CMD_ALLOC, CMD_CALL, CMD_READ, CMD_WRITE, CMD_QUIT };
enum arg_type { ARG_NONE, ARG_INT, ARG_PTR, ARG_BYTES };

struct bytearray {
        int num_bytes;
        char bytes[MAX_NUM_BYTES];
};

struct argument {
        enum arg_type type;
        union {
                int num;    
                uintptr_t addr;
                struct bytearray bytes;
        } val;
};

struct request {
        int command;
        int num_args;

        struct argument arg1;
        struct argument arg2;
        struct argument arg3;
        struct argument arg4;

        enum arg_type outarg_type;
};

struct response {
        int ok; 
        struct argument outarg; 
};

#endif
