/*
 * bypass_filter.c
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
#include "common.h"


/* bypass_filter.c
 *
 * This function checks the URL for known extensions
 */
int bypass_filter(char *urlptr) {
  char url[MAXINPUTSIZE];	/* URL placeholder */
  char *s;			/* search ptr */
  char *s2;			/* second search ptr */
  char **s3;			/* third search ptr */
  char *tmp;			/* MISC ptr */
  char *fqdn;			/* FQDN part of URL */
  char *ext;			/* Pointer to extension */

  /* Empty them */
  s = NULL;
  tmp = NULL;
  fqdn = NULL;

  /* Copy the URL to a local (this function) copy */
  strncpy(url,urlptr,MAXINPUTSIZE);

  /* From here on, urlptr should NOT be touched. We don't own it. */

  /* Make sure it is an HTTP or HTTPS URL */
  if (strncasecmp(url, "http:", 5) && strncasecmp(url, "https:", 5)) {
    return(0);
  } 

  /* It is HTTP or HTTPS so we can proceed */

  /* First, remove the url:// part
   *
   * Step one is to find the colon
   */
  tmp = strchr((char *)&url, ':');
  tmp += 3; /* add 3 to offset the :// part */

  /* The next step is to find a questionmark */
  s = strchr(tmp,'?');
  if (s != NULL) /* There is a questionmark */
    memset(s,'\0',1); /* Replace it with a NULL */

  /* Check to see if there is a username/password combination present
   *
   * This can be determined by finding an @ before a / character.
   *
   * (http://user:bla@website.com/index.html)
   */
  s = strchr(tmp, '@');
  if (s != NULL) { /* There is an '@' somewhere, find the first '/' */
    s2 = strchr(tmp, '/');
    if (s < s2) { /* The '@' is before the first '/' */
      fqdn = s + 1; /* The domain starts at the next charachter after @ */
    } else {
      fqdn = tmp; /* The domain starts immediately after the protocol */
    }
  } else { /* No '@' has been found, so fqdn = tmp */
    fqdn = tmp;
  }

  /* Check for any port assignments (http://bla.com:80/bla.html) */
  s = strchr(fqdn, ':');
  if (s != NULL) { /* There is a colon somewhere, find the first '/' */
    s2 = strchr(tmp, '/');
    if (s < s2) { /* The ':' is before the first '/' */
      tmp = s2 + 1; /* Set tmp to be the character after / */
      memset(s,'\0',1); /* Get rid of the colon */
    }
  } else { /* There is no colon */
    s = strchr(fqdn, '/');
    tmp = s + 1;
    memset(s,'\0',1); /* Get rid of the / */
  }

  /* At this point, fqdn is the hostname, tmp is the remainder of the url */

  /* Autmatically allow everything on www.wifinanny.com */
  if (!strncasecmp(fqdn, "www.wifinanny.com", strlen(fqdn))) {
    return(1);
  }

  /* From here, find the first occurence of a . from the right */
  s = strrchr(tmp,'.');
  if (s == NULL)  /* No extension found */
    return(0); /* FILTER ALL THE URLLLLLLLLLLLLLLLLLLLLLLLLLs \o/ */

  /* Point to the extension */
  ext = s + 1;

  /* We got the extensions, check it against a list */
  for (s3 = known_extensions; *s3; s3++) {
    if (strncasecmp(ext, *s3, strlen(*s3)) == 0) {
      return(1);
    }
  }

  /* The ext did not make the cut. Sorry, you're going to get filtered :) */
  return(0);
}

