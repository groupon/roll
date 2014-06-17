#!/bin/sh
# build.sh - recipe for creating a new roll package
# Andrew Ho (ho@groupon.com)
#
# These packages are required:
#
#     * autoconf
#     * binutils
#     * curl
#     * c_ares
#     * gcc
#     * libidn
#     * m4
#     * make
#     * openssl
#     * yaml
#     * zlib
#
# Copyright (c) 2013, Groupon, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# Neither the name of GROUPON nor the names of its contributors may be
# used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

PATH=/usr/local/bin:/usr/bin:/bin
ME=`basename "$0"`

# Convenient functions to display error messages
warn() { echo "$ME: $@" 1>&2; }
die() { echo "$ME: $@" 1>&2; exit 1; }

# If Makefile is left over, nuke it, but ignore errors
if [ -f Makefile ]; then
    warn "running make distclean to clear out old build..."
    make distclean || warn "make distclean returned error, continuing anyway"
fi

warn "running autoheader..."
autoheader || die "autoheader failed"

warn "running autoconf..."
autoconf || die "autoconf failed"

# Run configure
warn "running configure..."
./configure || die "configure failed"

# Run make
warn "running make..."
make || die "make failed"

exit 0
