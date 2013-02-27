roll - bootstrap or upgrade a Unix host with Roller
===================================================

This is the source code for the **roll** command line program, the part of
the **Roller** automated host management system which runs on a host, reads
hostclass and host YAML files from a Roller config server, and then
applies the changes locally to bring up or upgrade the local host.

Getting Started
---------------

To build roll from source:

    ./build.sh

Roller should build on any reasonably recent Unix system with a GNU
autoconf and gcc compilation stack, plus the
[libcurl](http://curl.haxx.se/libcurl/) and
[libyaml](http://pyyaml.org/wiki/LibYAML) libraries.

To bootstrap or upgrade a Unix host (assuming you have a Roller
configuration server available at <http://config/>), run the following
as superuser on the host to be upgraded:

    ./roll

Description
-----------

Roller is a host management system which arranges for software packages
and configuration files to be installed on a host in a reliable,
repeatable, and source controlled way. The following major components
comprise Roller:

* A data repository, with source controlled host and hostclass definitions
* Static, precompiled software packages
* A configuration server, with APIs to the read data and download packages
* The roll program, which uses the above to actually bootstrap a host

The roll program is the latter: it contacts a configuration server, asks
for host and hostclass definitions for the host that it is running on,
downloads and links packages according to the hostclass definition, and
then runs package hooks to generate configuration files and restart
services.

Roller uses [Encap](http://www.encap.org/) for its local host package
management, and [Git](http://git-scm.com) managed
[YAML](http://www.yaml.org/YAML) files for host and hostclass
definitions.

License
-------

    Copyright (c) 2013, Groupon, Inc.
    All rights reserved.
    
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are
    met:
    
    Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    
    Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    
    Neither the name of GROUPON nor the names of its contributors may be
    used to endorse or promote products derived from this software without
    specific prior written permission.
    
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
    IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
    PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
    TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
    LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
    NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Meta
----

* Home: <https://github.com/groupon/roll>
* Bugs: <https://github.com/groupon/roll/issues>
* Authors: <https://github.com/andrewgho>

### Prior Art ###

There are a number of existing host management solutions. The very
popular current ones are:

* [Chef](http://www.opscode.com/chef/)
* [Puppet](http://puppetlabs.com/)

Roller differs from these in its emphasis on simplicity and
repeatability, for example, by keeping all host and hostclass data in
static files, using prebuilt static packages that are locally served,
and striving to minimize the amount of deploy-time code that is run.
