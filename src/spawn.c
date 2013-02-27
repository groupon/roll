/* spawn.c - Spawns a child process.
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
#ifdef HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
    #include <sys/wait.h>
#endif
#ifdef HAVE_UNISTD_H
    #include <unistd.h>
#endif
#include "spawn.h"
#include "log.h"

#define MAX_ARGS 256
#define READ_BUFFER_SIZE 4096

/* Set a static environment for subprocesses */
static char *const DEFAULT_ENV[] = {
    "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
    NULL
};

int run_command(const char *command, ...) {
    int pipefd[2];
    pid_t pid;
    int status;
    va_list ap;
    char *argv[MAX_ARGS];
    int argc = 0;
    char buffer[READ_BUFFER_SIZE];
    int n;

    argv[argc++] = (char *)command; /* arg[0] is the command name */
    va_start(ap, command);
    while( (argc < MAX_ARGS) &&
           (argv[argc++] = va_arg(ap, char *)) != (char *)0) ;
    va_end(ap);
    if(argc == MAX_ARGS) {
        return -1;
    }

    if(pipe(pipefd) == -1) {
        perror("pipe");
        return -1;
    }

    pid = fork();
    if(pid == -1){ /* fork failure */

        perror("fork");
        close(pipefd[1]);
        return -1;

    } else if(pid == 0) { /* child; writes to pipe */

        close(pipefd[0]); /* close read end of pipe */
        dup2(pipefd[1], 1); /* connect stdout to pipe */
        dup2(pipefd[1], 2); /* connect stderr to pipe */
        close(pipefd[1]);

        execve(command, argv, DEFAULT_ENV);
        _exit(-1);

    } else { /* parent; reads from pipe */

        close(pipefd[1]); /* close write end of pipe */
        while( (n = read(pipefd[0], buffer, READ_BUFFER_SIZE - 1)) != 0) {
            buffer[n] = '\0';
            log_message("%s", buffer);
        }
        close(pipefd[0]);
        if(waitpid(pid, &status, 0) != pid) {
            perror("waitpid");
            return -1;
        }
        if(WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return -1;
        }

    }
}
