#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>



main(int argc, char* argv[])
{
	printf("Testing started\n");
	if(!fork())
	{
		printf("fork started\n");
		system(argv[1]);
		printf("system finished\n");
		sleep(2);
		return 0;
	}
	else
	{

		printf("fork started in controller\n");
        	int msgflg = IPC_CREAT | 0666;
        	key_t key;
        	size_t buflen;
		int msqid;
		int len;
		
		struct msgbuf
		{
    				long    mtype;
    				int     args[3];
		} sbuf;



        	key = 1234;

		printf("setting up msgq\n");
        	if ((msqid = msgget(key, msgflg )) < 0)   /* Get the message queue ID for the given key */
                	perror("msgget");


		printf("getting from msgq\n");
		len=sizeof(sbuf)-sizeof(int);
        	if (msgrcv(msqid, &sbuf, len,  0, 0) < 0)
        	{
			perror("msgrcv:");
        	}

		printf("Got message from Strata: type=%d, arg1=%x, arg2=%x, arg3=%x\n", (int)sbuf.mtype, sbuf.args[0], sbuf.args[1], sbuf.args[2]);

		sleep(2);
		return 0;

	}
}

