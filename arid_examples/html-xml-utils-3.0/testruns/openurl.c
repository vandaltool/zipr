/*
 * Routines to open a URL instead of a local file
 *
 * int openurl(const char *path)
 * FILE *fopenurl(const char *path)
 *
 * ToDo: Add arguments for PUT, POST; parse and return headers.
 *
 * Uses http_proxy and ftp_proxy environment variables.
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 7 March 1999
 * Version: $Id: openurl.c,v 1.8 2003/01/21 19:26:03 bbos Exp $
 */
#include <config.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif
#ifdef HAVE_ERRNO_H
#  include <errno.h>
#endif
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <string.h>
#include "export.h"
#include "heap.e"
#include "types.e"
#include "url.e"
#include "connectsock.e"

#define MAXBUF 4096				/* Max len of header lines */

static URL http_proxy = NULL, ftp_proxy = NULL;
static int http_proxy_init = 0, ftp_proxy_init = 0;


/* open_via_proxy -- open a URL via a proxy server */
static FILE *open_via_proxy(const char *url, const URL proxy)
{
  char line[MAXBUF];
  int fd, n;
  char *s;
  FILE *f;

  /* Connect */
  if ((fd = connectTCP(proxy->machine,
		       proxy->port ? proxy->port : (string)"80")) == -1)
    return NULL;

  /* Send request */
  newarray(s, strlen(url) + 18);
  n = sprintf(s, "GET %s HTTP/1.0\r\n\r\n", url);
  if (write(fd, s, n) != n) {close(fd); return NULL;}
  dispose(s);
#if 0
  shutdown(fd, 1);				/* No more output to server */
#endif

  /* Read headers */
  if (! (f = fdopen(fd, "r"))) {close(fd); return NULL;}
  clearerr(f);
  while (fgets(line, sizeof(line), f) && line[0] && line[0] != '\r' && line[0] != '\n') {
    /* ToDo: recombine continuation lines and store headers somewhere... */
    /* fprintf(stderr, "%s", line); */
  }
  if (ferror(f)) {fclose(f); return NULL;}

  /* Return the body of the stream */
  return f;
}

/* open_http -- open resource via HTTP; return file pointer or NULL */
static FILE *open_http(const URL url)
{
  char line[MAXBUF];
  int fd, n;
  char *s, *proxy;
  FILE *f;

  if (! http_proxy_init) {
    if ((proxy = getenv("http_proxy"))) http_proxy = URL_new(proxy);
    http_proxy_init = 1;
  }

  /* Check for proxy */
  if (http_proxy) return open_via_proxy(url->full, http_proxy);

  /* Connect */
  if ((fd = connectTCP(url->machine,
		       url->port ? url->port : (string)"80")) == -1)
    return NULL;

  /* Send request */
  newarray(s, strlen(url->full) + 16);		/* Should be long enough... */
  n = sprintf(s, "GET %s HTTP/1.0\r\n", url->path ? url->path : (string)"/");
  if (write(fd, s, n) != n) {close(fd); return NULL;}
  n = sprintf(s, "Host: %s:%s\r\n", url->machine,
	      url->port ? url->port : (string)"80");
  if (write(fd, s, n) != n) {close(fd); return NULL;}
  n = sprintf(s, "\r\n");
  if (write(fd, s, n) != n) {close(fd); return NULL;}
  dispose(s);
  shutdown(fd, 1);				/* No more output to server */

  /* Read headers */
  f = fdopen(fd, "r");
  while (fgets(line, sizeof(line), f) && line[0] && line[0] != '\r' && line[0] != '\n') {
    /* ToDo: recombine continuation lines and store headers somewhere... */
  }

  /* Return the body of the stream */
  return f;
}

/* open_ftp -- open resource via FTP; return file pointer or NULL */
static FILE *open_ftp(const URL url)
{
  char *proxy;

  if (! ftp_proxy_init) {
    if ((proxy = getenv("ftp_proxy"))) ftp_proxy = URL_new(proxy);
    ftp_proxy_init = 1;
  }
  if (ftp_proxy) return open_via_proxy(url->full, ftp_proxy);

  /* Can only work via proxy for now... */
  errno = ENOSYS;
  return NULL;
}

/* open_file -- open resource as local file or FTP; return file ptr or NULL */
static FILE *open_file(const URL url)
{
  FILE *f = NULL;

  if (! url->machine || eq(url->machine, "localhost")) {
    f = fopen(url->path, "r");
  }
  if (! f) f = open_ftp(url);
  return f;
}

/* fopenurl -- like fopen, but takes a URL; HTTP headers are parsed */
EXPORT FILE *fopenurl(char *path, char* mode)
{
  URL url;
  FILE *f;

  /* NOT IMPLEMENTED YET: mode = "w" (= PUT) and "a" (= POST) */
  if (! eq(mode, "r")) {errno = ENOSYS; return NULL;}

  url = URL_new(path);
  if (! url) {errno = EACCES; return NULL;}	/* Invalid URL */
  if (! url->proto) f = fopen(path, "r");	/* Assume it's a local file */
  else if (eq(url->proto, "http")) f = open_http(url);
  else if (eq(url->proto, "ftp")) f = open_ftp(url);
  else if (eq(url->proto, "file")) f = open_file(url);
  else {errno = EACCES; f = NULL;}		/* Unimplemented protocol */
  URL_dispose(url);
  return f;
}
