/* local_initd.h - parameters and source code for /etc/init.d/local_initd
 *
 * To output the sh source in LOCAL_INITD_SCRIPT as text:
 *
 *     (echo '#include <stdio.h>'; \
 *      cat local_initd.h; \
 *      echo 'int main() { fputs(LOCAL_INITD_SCRIPT, stdout); }') \
 *         | gcc -E - | gcc -xc - -owrite_local_initd \
 *         && ./write_local_initd > local_initd.sh
 *
 * And to turn sh source back into a safe #define:
 *
 *     perl -ne 'BEGIN { print qq(#define LOCAL_INITD_SCRIPT) }
 *         chomp; s/\\/\\\\/g; s/"/\\"/g; print qq( \\\n  "$_\\n");
 *         END { print "\n" }' local_initd.sh
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

#ifndef LOCAL_INITD_H
#define LOCAL_INITD_H

/* Filename to write the sh output to, and where to symlink that file to */
#define LOCAL_INITD_FILENAME "/etc/init.d/local_initd"
#define LOCAL_INITD_SYMLINKS { \
    "/etc/rc0.d/K00local_initd", \
    "/etc/rc1.d/K00local_initd", \
    "/etc/rc2.d/S99local_initd", \
    "/etc/rc3.d/S99local_initd", \
    "/etc/rc4.d/S99local_initd", \
    "/etc/rc5.d/S99local_initd", \
    "/etc/rc6.d/K00local_initd", \
    NULL, \
}

/* Gentoo/OpenRC puts symlinks in a single different place */
#define OPENRC_LOCAL_INITD_SYMLINK "/etc/runlevels/default/local_initd"

/* Actual contents of sh script to write to /etc/init.d/local_initd */
#define LOCAL_INITD_SCRIPT \
  "#!/bin/sh\n" \
  "# local_init.d - run scripts matching /usr/local/etc/rc.d/^[0-9][0-9].\n" \
  "# This file is automatically written by the roll program.\n" \
  "#\n" \
  "# Copyright (c) 2013, Groupon, Inc.\n" \
  "# All rights reserved.\n" \
  "#\n" \
  "# Redistribution and use in source and binary forms, with or without\n" \
  "# modification, are permitted provided that the following conditions are\n" \
  "# met:\n" \
  "#\n" \
  "# Redistributions of source code must retain the above copyright notice,\n" \
  "# this list of conditions and the following disclaimer.\n" \
  "#\n" \
  "# Redistributions in binary form must reproduce the above copyright\n" \
  "# notice, this list of conditions and the following disclaimer in the\n" \
  "# documentation and/or other materials provided with the distribution.\n" \
  "#\n" \
  "# Neither the name of GROUPON nor the names of its contributors may be\n" \
  "# used to endorse or promote products derived from this software without\n" \
  "# specific prior written permission.\n" \
  "#\n" \
  "# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS\n" \
  "# IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED\n" \
  "# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A\n" \
  "# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT\n" \
  "# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,\n" \
  "# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED\n" \
  "# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR\n" \
  "# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF\n" \
  "# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING\n" \
  "# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS\n" \
  "# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n" \
  "#\n" \
  "# chkconfig: 2345 99 00\n" \
  "# description: Starts and stop services via scripts in /usr/local/etc/rc.d\n" \
  "\n" \
  "# On Gentoo, run with runscript if we are not already in runscript\n" \
  "if [ -f /etc/gentoo-release -a -x /sbin/runscript -a \"x$SVCNAME\" = x ]; then\n" \
  "    exec /sbin/runscript \"$0\" $@\n" \
  "fi\n" \
  "\n" \
  "# Default environment\n" \
  "PATH=/usr/local/bin:/usr/bin:/bin\n" \
  "\n" \
  "# Default directory to iterate through and run init.d scripts in\n" \
  "DIR=/usr/local/etc/rc.d\n" \
  "\n" \
  "# Our name and usage, for error reporting; plus lockfile for CentOS\n" \
  "ME=`basename \"$0\"`\n" \
  "USAGE=\"usage: $ME {start|stop} [dir]\"\n" \
  "LOCKFILE=/var/lock/subsys/`echo \"$ME\" | sed 's/^\\(S99\\|K00\\)//'`\n" \
  "\n" \
  "# On Gentoo, ignore first filename argument\n" \
  "if [ -f /etc/gentoo-release ]; then\n" \
  "    arg_action=\"$2\"\n" \
  "    arg_dir=\"$3\"\n" \
  "else\n" \
  "    arg_action=\"$1\"\n" \
  "    arg_dir=\"$2\"\n" \
  "fi\n" \
  "\n" \
  "# Accept optional directory argument for testing, overriding $DIR\n" \
  "if [ \"x$arg_dir\" != x ]; then\n" \
  "    DIR=\"$arg_dir\"\n" \
  "fi\n" \
  "\n" \
  "# On Gentoo, declare that we need local directories, networking, a hostname\n" \
  "depend() {\n" \
  "    need localmount\n" \
  "    need net.lo\n" \
  "    after bootmisc hostname\n" \
  "}\n" \
  "\n" \
  "# On start, for CentOS, touch /var/lock/subsys sentinel; for Gentoo, return 0\n" \
  "start() {\n" \
  "    touch \"$LOCKFILE\"\n" \
  "    (run_local_initd start S)\n" \
  "    return 0\n" \
  "}\n" \
  "\n" \
  "# On stop, for CentOS, remove /var/lock/subsys sentinel\n" \
  "stop() {\n" \
  "    (run_local_initd stop K)\n" \
  "    rm -f \"$LOCKFILE\"\n" \
  "}\n" \
  "\n" \
  "# Run through each init.d script in order, passing it $action\n" \
  "run_local_initd() {\n" \
  "    action=\"$1\"\n" \
  "    prefix=\"$2\"\n" \
  "    status=0\n" \
  "    # Set IFS to handle filenames with spaces\n" \
  "    IFS=`/bin/echo -en \"\\n\\b\"`\n" \
  "    # Go to a safe directory\n" \
  "    cd /var/tmp\n" \
  "    # Match scripts to run (redirect stderr in case directory is empty)\n" \
  "    for f in `ls -1 \"$DIR\"/\"$prefix\"[0-9][0-9]?* 2>/dev/null`; do\n" \
  "        if [ -x \"$f\" ] && [ ! -d \"$f\" ]; then\n" \
  "            # Use env -i to ensure empty environment\n" \
  "            env -i \"PATH=$PATH\" \"$f\" \"$action\"\n" \
  "            status=`expr \"$status\" + \"0$?\"`\n" \
  "        else\n" \
  "            echo \"$ME: not an executable file: $f\" 1>&2\n" \
  "            status=`expr \"$status\" + 1`\n" \
  "        fi\n" \
  "    done\n" \
  "    return \"$status\"\n" \
  "}\n" \
  "\n" \
  "# Require \"start\" or \"stop\" argument, then run the corresponding function\n" \
  "if [ -f /etc/gentoo-release ]; then\n" \
  "    :\n" \
  "else\n" \
  "    status=0\n" \
  "    if [ \"x$arg_action\" = x ]; then\n" \
  "        echo \"$ME: missing required {start|stop} argument\" 1>&2\n" \
  "        echo \"$USAGE\" 1>&2\n" \
  "        exit 1\n" \
  "    elif [ \"x$arg_action\" = xstart ]; then\n" \
  "        start\n" \
  "        status=`expr \"$status\" + \"0$?\"`\n" \
  "    elif [ \"x$arg_action\" = xstop ]; then\n" \
  "        stop\n" \
  "    else\n" \
  "        echo \"$ME: first argument must be 'start' or 'stop'\" 1>&2\n" \
  "        echo \"$USAGE\" 1>&2\n" \
  "        exit 1\n" \
  "    fi\n" \
  "    exit \"$status\"\n" \
  "fi\n"

#endif /* ifndef LOCAL_INITD_H */
