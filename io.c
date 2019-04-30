/*
 * io.c
 *
 * (c) 2015 Cluecentral Inc.
 *
 * The source code of this program is proprietary to Cluecentral Inc, but
 * may be released into the Open Source domain upon completion.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <syslog.h>
#include <stdarg.h>
#include <netdb.h>
#include <unistd.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include "common.h"


/* query_connect
 *
 * This opens a UDP socket.
 *
 * DO NOT EXIT FROM THIS FUNCTION.
 *
 * If the thread needs to exit due to an error, it still needs to return
 * a response to the squid server. Only return 0 from here in case of 
 * an error.
 */
int query_connect(int sock, char *query_server) {
  struct hostent *hostp;
  struct sockaddr_in serveraddr;

  /* Initialize values */
  memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_port = htons(3768); /* 3768 is an official rblcheckd port */

  /* If unable to create a socket, return 0 */
  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    mylog("Error trying to create a socket.");
    return(0);
  }

  /* Resolve the query server or return 0 */
  hostp = gethostbyname(query_server);
  if(hostp == (struct hostent *)NULL) {
    mylog("Unable to find %s.", query_server);
    return(0);
  }
  memcpy(&serveraddr.sin_addr, hostp->h_addr, sizeof(serveraddr.sin_addr));

  /* This should always succeed, since it is UDP */
  if(connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
    mylog("Error connecting to %s (%d).", query_server, errno);
    if (sock)
     close(sock);
    return(0);
  }
  return(sock);
}










/* my_send()
 *
 * Send a message to an existing socket.
 *
 * DO NOT EXIT FROM THIS FUNCTION.
 *
 * If the thread needs to exit due to an error, it still needs to return
 * a response to the squid server. Only return 0 from here in case of
 * an error.
 */
int my_send(int sock, const char *fmt, ...) {
  va_list vl;
  char msg[MAXIOBUFSIZE];

  mylog("ms: 1\n");
  va_start(vl,fmt);
  vsnprintf(msg,sizeof(msg),fmt,vl);
  va_end(vl);

  mylog("ms: 2\n");
  if (sendto(sock, (char *) &msg, strlen(msg), 0, 0, 0) < 0) {
    mylog("Unable to send: %s", msg);
  }
  mylog("ms: 3\n");
  return(0);
}






























