/*
 * zeusclient.c
 *
 * (c) 2015 Cluecentral Inc.
 *
 * The source code of this program is proprietary to Cluecentral Inc, but
 * may be released into the Open Source domain upon completion.
 *
 * This is a first attempt at writing a url_rewrite helper for squid which
 * will contact query.wifinanny.com for permission to serve a url.
 *
 * The main routine does nothing more than listen on stdin and fork a pthread
 * as soon as a line has been received from squid.
 *
 * Squid sends a line in the form of:
 *
 * <id> <url> <client ip> <client mac> <request method> <server>, for example:
 * 0 http://www.a.com/a.ico 1.2.3.4 6e:3b:e9:1d:17:b2 GET wifinanny.com
 *
 * Squid expects a return of <id> <action>, where <action> can be OK or
 * a redirect.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "common.h"

/* main() does not take any parameters. */
int main() {
  char input[MAXINPUTSIZE];	/* input buffer*/
  FILE *fd;			/* File descriptor */
  char *ret;			/* Return value */
  pthread_t thread;		/* a place to go */

  setbuf(stdout, NULL);
  memset(serial, '\0', sizeof(serial));
  /* snprintf(serial, sizeof(serial), "12345"); */

  fd = fopen("/etc/config/SN", "r");
  if (fd == NULL) {
    fprintf(stdout,"Unable to open configuration file.\n");
    exit(0);
  }
  
  /* Read the serial number */
  ret = fgets((char *)&serial, sizeof(serial), (FILE *)fd);
  serial[strlen(serial)-1] = '\0';

  /* Initialize the mutex */
  pthread_mutex_init(&ts_input_lock, NULL);
  memset(input,'\0',MAXINPUTSIZE);

  mylog("Serial: %s\n", serial);

  /* Infinite while loop */
  while (1) {
    /* Start reading */
    if (fgets(input,MAXINPUTSIZE,stdin) == NULL) {

      /* If stdin closes, squid has died */
      mylog("I think stdin closed... Exiting: %d.", errno);
      exit(0);
    } else {
      /* Squid will send a terminating \n, let's remove it */
      if (input[strlen(input)-1] == '\n')
        input[strlen(input)-1] = '\0';

      /* Lock ts_input. Thread will unlock and free() */
      pthread_mutex_lock(&ts_input_lock);

      /* Fork a thread and process the request.
       */
      if(pthread_create(&thread, NULL, process_request, (void *) &input)) {
	pthread_mutex_unlock(&ts_input_lock);
	mylog("WARNING! Unable to create a thread for %s\n", input);
      }

      /* Detach the pthread to avoid a memory leak */
      pthread_detach(thread);

      /* Lock mutex and empty buffer */
      pthread_mutex_lock(&ts_input_lock);
      memset(input, '\0', MAXINPUTSIZE);
      /* mylog("MALLOC: freed %d for ptr %d", MAXINPUTSIZE, ts_input); */
      pthread_mutex_unlock(&ts_input_lock);

    }
  } /* while (fgets(input,MAXINPUTSIZE,stdin) != NULL) */

  /* We never get here... */
  mylog("WARNING! Exited while loop.");
  printf("Exiting\n");
  return(0);
} /* int main() */

