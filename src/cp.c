/* cp.c - Emulate cp
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
#ifdef HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif
#ifdef HAVE_FCNTL_H
    #include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
    #include <unistd.h>
#endif
#include <errno.h>

#define BUFSIZE 4096

int cp(char *src, char *dest, mode_t mode) {
    int src_fd = -1, dest_fd = -1, retval = 0, errno_orig;
    static char buf[BUFSIZE];
    ssize_t rcount, wcount;

    retval = 0;
    if((src_fd = open(src, O_RDONLY, 0)) == -1) {
        retval = 1;
        goto bail;
    }
    if((dest_fd = open(dest, O_WRONLY|O_TRUNC|O_CREAT, mode)) == -1) {
        retval = 1;
        goto bail;
    }
    while((rcount = read(src_fd, buf, BUFSIZE)) > 0) {
        wcount = write(dest_fd, buf, rcount);
        if(rcount != wcount || wcount == -1) {
            retval = 1;
            break;
        }
    }
    if(rcount < 0) {
        retval = 1;
        goto bail;
    }

  bail:
    errno_orig = errno;
    if(src_fd >= 0) close(src_fd);
    if(dest_fd >= 0) close(dest_fd);
    errno = errno_orig;
    return retval;
}
