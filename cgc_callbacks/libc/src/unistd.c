#include <unistd.h>

ssize_t write(int fd, const void* buf, size_t count)
{
	ssize_t ret=0;
	cgc_transmit(fd,buf,count,&ret);
	return ret;
	
}

