/*
 * Copyright (c) 2014 - Zephyr Software LLC
 *
 * This file may be used and modified for non-commercial purposes as long as
 * all copyright, permission, and nonwarranty notices are preserved.
 * Redistribution is prohibited without prior written consent from Zephyr
 * Software.
 *
 * Please contact the authors for restrictions applying to commercial use.
 *
 * THIS SOURCE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Author: Zephyr Software
 * e-mail: jwd@zephyr-software.com
 * URL   : http://www.zephyr-software.com/
 *
 */

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
