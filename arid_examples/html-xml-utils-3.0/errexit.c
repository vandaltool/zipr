/* errexit.c -- print message and exit
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 12 May 1998
 * Version: $Id: errexit.c,v 1.5 2003/01/21 19:44:39 bbos Exp $
 **/
#include <config.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_STRING_H
#  include <string.h>
#elif HAVE_STRINGS_H
#  include <strings.h>
#endif
#include "export.h"

/* errexit -- print message and exit */
EXPORT int errexit(char *format,...)
{
  va_list ap;

  va_start(ap, format);
  vfprintf(stderr, format, ap);
  va_end(ap);
  exit(1);
}


