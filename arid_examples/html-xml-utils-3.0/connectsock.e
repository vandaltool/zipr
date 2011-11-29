 u_short portbase ;
 int connectsock(const char *host, const char *service, char *protocol);
 int connectTCP(const char *host, const char *service);
 int connectUDP(char *host, char *service);
 int passivesock(char *service, char *protocol, int qlen);
 int passiveTCP(char *service, int qlen);
 int passiveUDP(char *service);
