/* local_profiled.h - source code for /etc/profile.d/local_profiled.sh
 *
 * To output the sh source in LOCAL_PROFILED_SCRIPT as text:
 *
 *     (echo '#include <stdio.h>'; \
 *      cat local_profiled.h; \
 *      echo 'int main() { fputs(LOCAL_PROFILED_SCRIPT, stdout); }') \
 *         | gcc -E - | gcc -xc - -owrite_local_profiled \
 *         && ./write_local_profiled > local_profiled.sh
 *
 * And to turn sh source back into a safe #define:
 *
 *     perl -ne 'BEGIN { print qq(#define LOCAL_PROFILED_SCRIPT) }
 *         chomp; s/\\/\\\\/g; s/"/\\"/g; print qq( \\\n  "$_\\n");
 *         END { print "\n" }' local_profiled.sh
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

#ifndef LOCAL_PROFILED_H
#define LOCAL_PROFILED_H

/* Filename to write the sh output to */
#define LOCAL_PROFILED_FILENAME "/etc/profile.d/local_profiled.sh"

/* Actual contents of sh script to write to /etc/init.d/local_profiled.sh */
#define LOCAL_PROFILED_SCRIPT \
  "# local_profiled.sh - run scripts matching /usr/local/etc/profile.d/*.sh\n" \
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
  "\n" \
  "for i in /usr/local/etc/profile.d/*.sh ; do\n" \
  "    if [ -r \"$i\" ]; then\n" \
  "        if [ \"$PS1\" ]; then\n" \
  "            . $i\n" \
  "        else\n" \
  "            . $i >/dev/null 2>&1\n" \
  "        fi\n" \
  "    fi\n" \
  "done\n"

#endif /* ifndef LOCAL_PROFILED_H */
