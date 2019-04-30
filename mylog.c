/*
 * mylog.c
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


/* mylog
 *
 * This is a simple syslog function
 */
void mylog(const char *fmt, ...) {
  va_list vl;

  va_start(vl,fmt);
  openlog("zeusclient", LOG_CONS | LOG_PID, LOG_USER);
  vsyslog(LOG_DEBUG, fmt, vl);
  va_end(vl);
  closelog();
}

