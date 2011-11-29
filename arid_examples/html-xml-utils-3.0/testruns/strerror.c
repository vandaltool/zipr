/*
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 31 Mar 2000
 * Version: $Id: strerror.c,v 1.2 2003/01/21 19:26:03 bbos Exp $
 **/
#include <config.h>
#ifdef HAVE_ERRNO_H
#  include <errno.h>
#endif
#include "export.h"

#ifndef HAVE_STRERROR
/* strerror -- return a string describing the error number */
EXPORT char *strerror(int errnum)
{
  return errnum < sys_nerr ? sys_errlist[errnum] : "Unknown error";
}
#endif /* HAVE_STRERROR */
