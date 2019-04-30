/*
 * common.h
 *
 * (c) 2015 Cluecentral Inc.
 *
 * The source code of this program is proprietary to Cluecentral Inc, but
 * may be released into the Open Source domain upon completion.
 *
 */

/* Global defines */
#define MAXINPUTSIZE		4096	/* maximum input buffer for fgetc */
#define MAXIOBUFSIZE		16384	/* maximum buffer size for io */
#define MAXB64			8192	/* just to be on the safe side */
#define PROTOCOL		"1"	/* internal protocol version */
#define RESPONSE_TIMEOUT	15	/* timeout of the response */
#define RESPONSE_OK		"ERR"	/* URL is permitted */
#define DEBUG

/* Global functions */
void *process_request();
void b64_encode();
void mylog(const char *, ...);
int my_send(int, const char *, ...);
int bypass_filter (char *);
int query_connect();

/* Global variables */
pthread_mutex_t ts_input_lock;		/* Mutex lock for input buffer */
char serial[64];			/* Serial number of device */

/* Extensions which do not need any processing */
static char *known_extensions[] = {
  "js",
  "css",
  "ico",
  "woff",
  "woff2",
  "json",
  "xml"
};
