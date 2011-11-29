/* connectsock.c
 *
 * Copyright © 1994-2000 World Wide Web Consortium
 * See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
 *
 * Author: Bert Bos <bert@w3.org>
 * Created: 12 May 1998
 * Version: $Id: connectsock.c,v 1.6 2003/01/21 19:26:03 bbos Exp $
 **/

#include <config.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef HAVE_ARPA_INET_H
#  include <arpa/inet.h>
#endif
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "export.h"

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

EXPORT u_short portbase = 0;			/* for non-root servers */


/* connectsock -- allocate & connect a socket using TCP or UDP */
EXPORT int connectsock(const char *host, const char *service, char *protocol)
{
  /* host = name of host to which connection is desired		*/
  /* service = service associated with the desired port		*/
  /* protocol = name of protocol to use ("tcp" or "udp")	*/
  struct hostent *phe;				/* ptr to host info entry */
  struct servent *pse;				/* ptr to service info entry */
  struct protoent *ppe;				/* ptr protocol info entry */
  struct sockaddr_in sin;			/* Internet endpoint address */
  int s, type;					/* socket desc & socket type */

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  errno = 0;

  /* Map service to port number */
  if ((pse = getservbyname(service, protocol)))
    sin.sin_port = pse->s_port;
  else if ((sin.sin_port = htons(atoi(service))) == 0)
    { if (!errno) errno = ENOSYS; return -1; }	/* can't get service entry */

  /* Map host name to IP address, allowing for dotted decimal */
  if ((phe = gethostbyname(host)))
    memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
  else if ((sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
    { if (!errno) errno = ENXIO; return -1; }	/* can't get host entry */

  /* Map protocol name to protocol number */
  if ((ppe = getprotobyname(protocol)) == 0) return -1;

  /* Use protocol to choose a socket type */
  type = (strcmp(protocol, "udp") == 0) ? SOCK_DGRAM : SOCK_STREAM;

  /* Allocate a socket */
  if ((s = socket(PF_INET, type, ppe->p_proto)) < 0) return -1;

  /* Connect the socket */
  if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) return -1;

  return s;
}

/* connectTCP -- connect to a specified UDP service on a specified host */
EXPORT int connectTCP(const char *host, const char *service)
{
  return connectsock(host, service, "tcp");
}

/* connectUDP -- connect to a specified UDP service on a specified host */
EXPORT int connectUDP(char *host, char *service)
{
  return connectsock(host, service, "udp");
}

/* passivesock -- allocate & bind a server socket using TCP or UDP */
EXPORT int passivesock(char *service, char *protocol, int qlen)
{
  /* service = service associated with the desired port		*/
  /* protocol = name of protocol to use ("tcp" or "udp")	*/
  /* qlen = maximum length of the server request queue		*/
  struct servent *pse;				/* ptr to service info entry */
  struct protoent *ppe;				/* ptr protocol info entry */
  struct sockaddr_in sin;			/* Internet endpoint address */
  int s, type;					/* socket desc & socket type */

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  errno = 0;

  /* Map service to port number */
  if ((pse = getservbyname(service, protocol)))
    sin.sin_port = htons(ntohs((u_short)pse->s_port) + portbase);
  else if ((sin.sin_port = htons((u_short)atoi(service))) == 0)
    { if (!errno) errno = ENOSYS; return -1; }	/* can't get service entry */

  /* Map protocol name to protocol number */
  if ((ppe = getprotobyname(protocol)) == 0) return -1;

  /* Use protocol to choose a socket type */
  type = (strcmp(protocol, "udp") == 0) ? SOCK_DGRAM : SOCK_STREAM;

  /* Allocate a socket */
  if ((s = socket(PF_INET, type, ppe->p_proto)) < 0) return -1;

  /* Bind the socket */
  if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) return -1;
  if (type == SOCK_STREAM && listen(s, qlen) < 0) return -1;

  return s;
}

/* passiveTCP -- creat a passive socket for use in a TCP server */
EXPORT int passiveTCP(char *service, int qlen)
{
  /* service = service associated with thte desired port	*/
  /* qlen = maximum server request queue length			*/
  return passivesock(service, "tcp", qlen);
}

/* passiveUDP -- creat a passive socket for use in a UDP server */
EXPORT int passiveUDP(char *service)
{
  return passivesock(service, "udp", 0);
}
