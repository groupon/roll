/* config_parse.h - Parsers for hostclass and host configuration files.
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

#ifndef CONFIG_PARSE_H
#define CONFIG_PARSE_H

#ifdef __cplusplus
extern "C" {
#endif

#define FAILSAFE_GROUP_NAME "failsafe"
#define MAX_VALUE_SIZE 256

typedef struct host_config_s {
    unsigned char hostclass_tag[MAX_VALUE_SIZE];
} host_config_t;

typedef struct package_spec_s {
    unsigned char group[MAX_VALUE_SIZE];
    unsigned char package_name[MAX_VALUE_SIZE];
    struct package_spec_s *next;
} package_spec_t;

typedef struct os_image_spec_s {
    unsigned char hardware_tag[MAX_VALUE_SIZE];
    unsigned char image_name[MAX_VALUE_SIZE];
    struct os_image_spec_s *next;
} os_image_spec_t;

typedef struct hostclass_config_s {
    unsigned char host_tag[MAX_VALUE_SIZE];
    os_image_spec_t *os_image_list;
    package_spec_t *package_list;
    char has_failsafe;
} hostclass_config_t;

int parse_host_config(host_config_t *host_config, FILE *host_file);
int parse_hostclass_config(hostclass_config_t *hostclass_config, FILE *hostclass_file);
void free_hostclass_config(hostclass_config_t *hostclass_config);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef CONFIG_PARSE_H */
