/*
 * process_request.c
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
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "common.h"



/* process_request
 *
 * This is the initial function called by pthread_create()
 *
 * We split the main argument up into pieces and send off the authentication
 * request.
 *
 * Upon completion, we exit the thread.
 */
void *process_request(void *arg) {
  char query[MAXIOBUFSIZE];		/* the query string to the server */
  char input[MAXINPUTSIZE];		/* full input string */
  char msg[MAXIOBUFSIZE];		/* message out */
  char b64_url[MAXB64];			/* URL in Base64*/
  char b64_client_ip[MAXB64];		/* client IP address in Base64 */
  char b64_client_mac[MAXB64];		/* client MAC address in Base64*/
  char b64_serial[MAXB64];		/* device serial in Base64 */
  char *url;				/* url */
  char *client_ip;			/* client IP addresses */
  char *client_mac;			/* client MAC address */
  char *channel_id; 			/* needed for squid */
  char *response_thread_id; 		/* needed to validate */
  char *response_text;			/* response text */
  char *method;				/* HTTP method (ignored for now */
  char *query_server;			/* The server to query */
  char *space = " ";			/* Needed for strtok */
  char *delim = "\\";			/* Needed for strtok */
  int socket = 0;			/* FD for socket */
  int recvlen = 0;			/* Return value */
  struct timeval tv;			/* Timeout */
  struct sockaddr_in remaddr;		/* Remote sockaddr */
  socklen_t addrlen = sizeof(remaddr);  /* length */
  pthread_t tid = pthread_self();	/* Thread ID */
  
  /* Empty vars */
  memset(input,'\0',sizeof(input));
  memset(msg,'\0',sizeof(msg));
  memset(query,'\0',sizeof(query));

  /* Safely copy the string into a new buffer */
  strncpy((char *)input,arg,MAXINPUTSIZE);
  

  /* Free mutex so main thread can free() */
  pthread_mutex_unlock(&ts_input_lock);

  /* Chop it up */
  channel_id = strtok(input, space);
  url = strtok(NULL, space);
  client_ip = strtok(NULL, space);
  client_mac = strtok(NULL, space);
  method = strtok(NULL, space);
  query_server = strtok(NULL, space);

  if (channel_id == NULL ||
      url == NULL ||
      client_ip == NULL ||
      client_mac == NULL ||
      method == NULL ||
      query_server == NULL) {
    mylog("Ignoring invalid request %s", input);
    fprintf(stdout,"%s %s\n", channel_id, RESPONSE_OK);
    pthread_exit(0);
  }

  /* Check URL parser for the file extension
   *
   * If the URL points to a file of certain extensions, don't process it
   * and allow it automatically. This can safely be done when the following
   * extensions are requested:
   *
   * .js 	Nothing but Javascript
   * .css	Cascading Style Sheets
   * .ico	Small icons
   * .woff	Font files
   * .woff2	Font files
   * .json	JSON reply	
   * .xml	XML reply
   *
   * They are listed in common.h as well.
   */
  if (bypass_filter(url)) { /* bypass_filter = true */
    mylog("bypassing %s\n", url);
    fprintf(stdout,"%s %s\n", channel_id, RESPONSE_OK); /* send response */
    if (socket)
      close(socket);	/* close socket */
    pthread_exit(0);	/* exit thread */
  }

  /* Convert to Base64 */
  b64_encode(url, &b64_url);
  b64_encode(serial, &b64_serial);
  b64_encode(client_ip, &b64_client_ip);
  b64_encode(client_mac, &b64_client_mac);

  /* At this point, we have the following data
   *
   * - Squid's Channel ID in channel_id;
   * - Our thread ID in tid;
   * - The client's IP address in client_ip
   * - The client_ip encoded in base64 in b64_client_ip
   * - The client's MAC address in client_mac
   * - The client_mac encoded in base64 in b64_client_mac
   * - The URL in url
   * - The url encoded in base64 in b64_url
   */

  /* Open up a socket to the query server */
  socket = query_connect(socket, query_server);
  if (!socket) {
    fprintf(stdout,"%s %s\n", channel_id, RESPONSE_OK);
    pthread_exit(0);
  }

  /* Construct the query. This is protocol version 1, which looks like:
   * <protocol>:<channel_id>:<serial>:<mac b64>:<ip b64>:<url b64>
   *
   * To make sure we get the right answer, we use our thread ID as
   * channel ID.
   */
  snprintf(query, sizeof(query), "%s:%u:%s:%s:%s:%s",
      PROTOCOL,
      (unsigned int)tid,
      b64_serial,
      b64_client_mac,
      b64_client_ip,
      b64_url);

  /* Send the query */
  if(sendto(socket, (char *) &query, strlen(query), 0, 0, 0) < 0)
    mylog("error: unable to send msg (%s)\n",query);

  /* No longer need the contents */
  memset(query,'\0',sizeof(query));

  /* Set the timeout on the socket */
  tv.tv_sec = RESPONSE_TIMEOUT;	
  tv.tv_usec = 0;
  if (setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    mylog("Unable to set timeout on socket, exiting thread.");
    close(socket);
    fprintf(stdout,"%s %s\n", channel_id, RESPONSE_OK);
    pthread_exit(0);
  }

  /* Let's wait for a response */
  recvlen = recvfrom(socket, &query, MAXIOBUFSIZE, 0,
      (struct sockaddr *)&remaddr, &addrlen);

  if (recvlen < 1) {
    mylog("Did not receive an answer in time.");
    fprintf(stdout,"%s %s\n", channel_id, RESPONSE_OK);
    if (socket)
      close(socket);
    pthread_exit(0);
  }

  /* Validate the response
   *
   * The response may contain spaces, so we use \ as delimiter
   */
  response_thread_id = strtok(query, delim);
  response_text  = strtok(NULL, delim);

  /* Check for sanity */
  if (response_thread_id == NULL ||
      response_text == NULL) {
    mylog("Ignoring invalid reply %s", query);
    fprintf(stdout,"%s %s\n", channel_id, RESPONSE_OK);
    pthread_exit(0);
  }

  /* Validate the thread_id */
  if ((unsigned int)tid != (unsigned int)atoi(response_thread_id)) {
    mylog("Ignoring invalid thread ID: %u, reponse_channel_id: %s!",
	tid, response_thread_id);
    fprintf(stdout,"%s %s\n", channel_id, RESPONSE_OK);
    pthread_exit(0);
  } 

  /* Forward the response to the proxy */
  fprintf(stdout,"%s %s\n", channel_id, response_text);

  /* Close our socket */
  if (socket)
    close(socket);

  /* Exit the thread */
  pthread_exit(0);
} /* process_request(void *threadarg) */


