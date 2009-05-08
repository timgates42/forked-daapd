/*
 * Copyright (C) 2009 Julien BLACHE <jb@jblache.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#include "logger.h"


static pthread_mutex_t logger_lck = PTHREAD_MUTEX_INITIALIZER;
static int logdomains;
static int threshold;
static int console;
static char *logfilename;
static FILE *logfile;
static char *labels[] = { "config", "daap", "db", "httpd", "main", "mdns", "misc", "parse", "rsp", "scan", "xcode", "lock" };


static int
set_logdomains(char *domains)
{
  char *ptr;
  char *d;
  int i;

  logdomains = 0;

  while ((d = strtok_r(domains, " ,", &ptr)))
    {
      domains = NULL;

      for (i = 0; i < N_LOGDOMAINS; i++)
	{
	  if (strcmp(d, labels[i]) == 0)
	    {
	      logdomains |= (1 << i);
	      break;
	    }
	}

      if (i == N_LOGDOMAINS)
	{
	  fprintf(stderr, "Error: unknown log domain '%s'\n", d);
	  return -1;
	}
    }

  return 0;
}

void
DPRINTF(int severity, int domain, char *fmt, ...)
{
  va_list ap;
  char stamp[32];
  time_t t;
  int ret;

  if (!((1 << domain) & logdomains) || (severity > threshold))
    return;

  pthread_mutex_lock(&logger_lck);

  if (!logfile && !console)
    {
      pthread_mutex_unlock(&logger_lck);
      return;
    }

  if (logfile)
    {
      t = time(NULL);
      ret = strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", localtime(&t));
      if (ret == 0)
	stamp[0] = '\0';

      fprintf(logfile, "[%s] %6s: ", stamp, labels[domain]);

      va_start(ap, fmt);
      vfprintf(logfile, fmt, ap);
      va_end(ap);
    }

  if (console)
    {
      fprintf(stderr, "%6s: ", labels[domain]);

      va_start(ap, fmt);
      vfprintf(stderr, fmt, ap);
      va_end(ap);
    }

  pthread_mutex_unlock(&logger_lck);
}

void
logger_reinit(void)
{
  FILE *fp;

  pthread_mutex_lock(&logger_lck);

  if (logfile)
    {
      fclose(logfile);

      fp = fopen(logfilename, "a");
      if (!fp)
	{
	  fprintf(logfile, "WARNING: Could not reopen logfile, logging will stop now\n");
	}

      fclose(logfile);
      logfile = fp;
    }

  pthread_mutex_unlock(&logger_lck);
}


/* The functions below are used at init time with a single thread running */
void
logger_domains(void)
{
  int i;

  fprintf(stdout, "%s", labels[0]);

  for (i = 1; i < N_LOGDOMAINS; i++)
    fprintf(stdout, ", %s", labels[i]);

  fprintf(stdout, "\n");
}

void
logger_detach(void)
{
  console = 0;
}

int
logger_init(char *file, char *domains, int severity)
{
  int ret;

  if ((sizeof(labels) / sizeof(labels[0])) != N_LOGDOMAINS)
    {
      fprintf(stderr, "WARNING: log domains do not match\n");

      return -1;
    }

  console = 1;
  threshold = severity;

  if (domains)
    {
      ret = set_logdomains(domains);
      if (ret < 0)
	return ret;
    }
  else
    logdomains = ~0;

  if (!file)
    return 0;

  logfile = fopen(file, "a");
  if (!logfile)
    {
      fprintf(stderr, "Could not open logfile %s: %s\n", file, strerror(errno));

      return -1;
    }

  logfilename = file;

  return 0;
}

void
logger_deinit(void)
{
  if (logfile)
    fclose(logfile);
}