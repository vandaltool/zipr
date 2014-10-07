#ifndef  cgc_h
#define  cgc_h

void cgc_terminate(int status_);
int cgc_transmit(int fd, const void *buf, size_t count, size_t *tx_bytes);
int cgc_receive(int fd, void *buf, size_t count, size_t *rx_bytes);
int cgc_fdwait(int nfds, fd_set *readfds, fd_set *writefds, const struct timeval *timeout, int *readyfds);
int cgc_allocate(size_t length, int is_X, void **addr);
int cgc_deallocate(void *addr, size_t length);
int cgc_random(void *buf, size_t count, size_t *rnd_bytes);

#endif
