/* log.c - A simple logging facility.
 *
 * Copyright (c) 2013, Groupon, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of GROUPON nor the names of its contributors may be
 * used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#ifdef HAVE_UNISTD_H
    #include <unistd.h>
#endif
#ifdef HAVE_SYS_STAT_H
    #include <sys/stat.h>
#endif
#ifdef HAVE_STRING_H
    #include <string.h>
#endif
#ifdef HAVE_LIMITS_H
    #include <limits.h>
#endif
#include <errno.h>
#include <time.h>
#include <assert.h>
#include "log.h"

#define LOG_FILE_FORMAT "roll-%04d%02d%02dT%02d%02d%02d.log"
#define LOG_FILE_LINK_NAME "roll.log"

static FILE *logfile = NULL;
static char log_filename[PATH_MAX] = "";

static void log_timestamp() {
    time_t t;
    struct tm ltm;

    assert(sizeof(t) == 8);
    t = time(NULL);
    localtime_r(&t, &ltm);
    if(logfile)
        fprintf(logfile,
                "[%04d-%02d-%02d %02d:%02d:%02d %s] ",
                ltm.tm_year + 1900,
                ltm.tm_mon + 1,
                ltm.tm_mday,
                ltm.tm_hour,
                ltm.tm_min,
                ltm.tm_sec,
                ltm.tm_zone);
    fprintf(stderr,
            "[%04d-%02d-%02d %02d:%02d:%02d %s] ",
            ltm.tm_year + 1900,
            ltm.tm_mon + 1,
            ltm.tm_mday,
            ltm.tm_hour,
            ltm.tm_min,
            ltm.tm_sec,
            ltm.tm_zone);
}

int log_init(char *filename, char *linkname) {
    int n;
    struct stat statbuf;
    time_t t;
    struct tm ltm;
    char build_filename[PATH_MAX];
    char build_linkname[PATH_MAX];
    char *linktarget;

    if(filename == NULL || !*filename) {
        filename = build_filename;
        if(linkname == NULL || !*linkname) linkname = build_linkname;

        /* Check for LOG_DIR/LOG_SUBDIR, make it if necessary */
        n = snprintf(filename,
                     PATH_MAX,
                     "%s/%s",
                     LOG_DIR_ROOT,
                     LOG_DIR_SUBDIR);
        if(n == PATH_MAX) {
            fprintf(stderr, "Log directory name %s/%s is too long.\n", LOG_DIR_ROOT, LOG_DIR_SUBDIR);
            return 0;
        }

        if(-1 == stat(filename, &statbuf)) {
            if(ENOENT == errno) {
                if(0 != mkdir(filename, 0755)) {
                    perror(filename);
                    return 0;
                }
            } else {
                perror(filename);
                return 0;
            }
        }

        /* The log file has a time stamp */
        assert(sizeof(t) == 8);
        t = time(NULL);
        localtime_r(&t, &ltm);
        strncat(filename + n, "/", PATH_MAX - n); n++;
        n += snprintf(filename + n,
                      PATH_MAX - n,
                      LOG_FILE_FORMAT,
                      ltm.tm_year + 1900,
                      ltm.tm_mon + 1,
                      ltm.tm_mday,
                      ltm.tm_hour,
                      ltm.tm_min,
                      ltm.tm_sec);
        if(n == PATH_MAX) {
            fprintf(stderr, "Log filename is too long.\n");
            return 0;
        }
    }

    /* Open the file */
    logfile = fopen(filename, "w");
    if(NULL == logfile) {
        fprintf(stderr, "Cannot open %s for writing.\n", filename);
        return 0;
    }
    strcpy(log_filename, filename);

    /* Swap the symlink */
    if(linkname != NULL) {
        n = snprintf(linkname,
                     PATH_MAX,
                     "%s/%s",
                     LOG_DIR_ROOT,
                     LOG_FILE_LINK_NAME);
        if(n == PATH_MAX) {
            fprintf(stderr, "Log filename symlink %s/%s is too long.\n", LOG_DIR_ROOT, LOG_FILE_LINK_NAME);
            return 0;
        }
        linktarget = filename + strlen(LOG_DIR_ROOT) + 1;
        unlink(linkname); /* ignore error */
        if(symlink(linktarget, linkname)) {
            fprintf(stderr, "Cannot create symlink %s -> %s.\n", linkname, linktarget);
            return 0;
        }
    }

    return 1;
}

const char *get_log_filename() {
    return log_filename;
}

void log_close() {
    if(logfile)
        fclose(logfile);
    logfile = NULL;
}

void log_message(const char *format, ...) {
    va_list ap, ap2;
    va_start(ap, format);
    va_copy(ap2, ap);
    if(logfile)
        vfprintf(logfile, format, ap);
    vfprintf(stderr, format, ap2);
    va_end(ap);
    va_end(ap2);
}

void log_info(const char *format, ...) {
    va_list ap, ap2;
    log_timestamp();
    va_start(ap, format);
    va_copy(ap2, ap);
    if(logfile) {
        vfprintf(logfile, format, ap);
        fprintf(logfile, "\n");
    }
    vfprintf(stderr, format, ap2);
    fprintf(stderr, "\n");
    va_end(ap);
    va_end(ap2);
}

void log_error(const char *format, ...) {
    va_list ap, ap2;
    log_timestamp();
    va_start(ap, format);
    va_copy(ap2, ap);
    if(logfile) {
        vfprintf(logfile, format, ap);
        fprintf(logfile, "\n");
    }
    vfprintf(stderr, format, ap2);
    fprintf(stderr, "\n");
    va_end(ap);
    va_end(ap2);
}

void log_header(const char *message, int failsafe_mode) {
    char buf[256];
    int i, length;

    if(failsafe_mode) {
        length = snprintf(buf, 256, "== FAILSAFE MODE %s ", message);
    } else {
        length = snprintf(buf, 256, "== %s ", message);
    }

    if(logfile) {
        fprintf(logfile, "\n%s", buf);
    }
    fprintf(stderr, "\n%s", buf);

    for(i = 0; i < 72 - length; i++) {
        if(logfile)
            fprintf(logfile, "=");
        fprintf(stderr, "=");
    }
    if(logfile)
        fprintf(logfile, "\n");
    fprintf(stderr, "\n");
}

