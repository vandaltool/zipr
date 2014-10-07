#include <unistd.h>
#include <syscall.h>

ssize_t write(int fd, const void* buf, size_t count)
{
#ifdef CGC
	ssize_t ret=0;
	cgc_transmit(fd,buf,count,&ret);
	return ret;
#else
	syscall(SYS_write,fd,buf,count);
#endif
	
}

