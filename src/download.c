/* download.c - Downloads a file via HTTP.
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
#include <string.h>
#include <curl/curl.h>
#include "download.h"
#include "log.h"

static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written;
    written = fwrite(ptr, size, nmemb, stream);
    return written;
}

/*
 * proxy is optional: if NULL then don't set the proxy option
 *                    if non-NULL, then it should be a string of the
 *                    form: "proxyhostname[:portnumber]"
 */
int download(const char *source_url, const char *dest_file, const char *proxy) {
    CURL *curl = NULL;
    CURLcode rc;
    FILE *fp = NULL;
    long status;
    int result = 0;
    char error_buffer[1024] = { 0 };

    if( !(fp = fopen(dest_file, "wb")) ) {
        log_error("  Cannot open %s for writing.", dest_file);
        goto error;
    }

    curl = curl_easy_init();
    if(!curl) {
        log_error("  Cannot initialze curl.");
        goto error;
    }

    if (proxy && strlen(proxy) > 0) {
      curl_easy_setopt(curl, CURLOPT_PROXY, proxy);
    }
    curl_easy_setopt(curl, CURLOPT_URL, source_url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
    rc = curl_easy_perform(curl);
    if(CURLE_OK == rc) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
        if(200 == status ||
           (strncmp(source_url, "file:", strlen("file:")) == 0 && 0 == status))
        {
            result = 1;
        } else {
            log_error("  Download failed with %d HTTP result code.", status);
        }
    } else {
        log_error("  Download failed with %d return code from curl_easy_perform().", rc);
        log_error("  Error recorded by libcurl: %s\n", error_buffer);
        if (CURLE_WRITE_ERROR == rc) {
            log_error("  The disk might be full.\n");
        }
    }
error:
    if(fp)
        fclose(fp);
    if(curl)
        curl_easy_cleanup(curl);
    return result;
}
