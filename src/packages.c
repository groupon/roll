/* packages.c - Package management functions.
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
#include <stdlib.h>
#ifdef HAVE_SYS_TYPES_H
    #include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
    #include <unistd.h>
#endif
#ifdef HAVE_STRING_H
    #include <string.h>
#endif
#ifdef HAVE_SYS_STAT_H
    #include <sys/stat.h>
#endif
#include <dirent.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include "packages.h"
#include "log.h"
#include "spawn.h"
#include "download.h"
#include "rmrf.h"

static int extract_package(const char *package_archive,
                           const char *extract_dir)
{
    char cwd[PATH_MAX];
    int exit_code;

    if(!getcwd(cwd, PATH_MAX)) {
        log_error("  Could not get current working directory");
        return 0;
    }

    if(0 != chdir(extract_dir)) {
        log_error("  Failed to change directory to %s: %s",
                  extract_dir,
                  strerror(errno));
        return 0;
    }

    exit_code =
        run_command("/bin/tar",
                    "xzf",              /* eXtract gZipped File */
                    package_archive,
                    "-p",               /* preserve permissions */
                    "-o",               /* strip ownership */
                    NULL                /* no more args */
         );
    if(0 != exit_code) {
        log_error("  Failed to extract package %s (tar exited with status %d)", package_archive, exit_code);
        return 0;
    }

    if(0 != chdir(cwd)) {
        log_error("  Failed to change directory to %s: %s",
                  cwd,
                  strerror(errno));
        return 0;
    }

    return 1;
}

int download_packages(const package_spec_t *package_list,
                      const char *package_groups[],
                      const char *download_url_format,
                      const char *package_stow_dir,
                      const char *package_download_dir,
                      const char *package_temp_dir,
                      const char *proxy)
{
    const package_spec_t *current_package;
    struct stat st;
    char downtemp[PATH_MAX],
         down[PATH_MAX],
         temp[PATH_MAX],
         final[PATH_MAX],
         url[PATH_MAX];
    int g, in_group;
    int n = 0, result = 0;

    for(current_package = package_list;
        current_package;
        current_package = current_package->next) {

        for(g = 0, in_group = 0; package_groups[g] != NULL && !in_group; g++) {
            if(!strcmp(package_groups[g], (char *)current_package->group)) {
                in_group = 1;
            }
        }

        if(in_group) {
            snprintf(downtemp,
                     PATH_MAX,
                     "%s/%s.tar.gz.%ld",
                     package_download_dir,
                     current_package->package_name,
                     (long)getpid());
            snprintf(down,
                     PATH_MAX,
                     "%s/%s.tar.gz",
                     package_download_dir,
                     current_package->package_name);
            snprintf(temp,
                     PATH_MAX,
                     "%s/%s",
                     package_temp_dir,
                     current_package->package_name);
            snprintf(final,
                     PATH_MAX,
                     "%s/%s",
                     package_stow_dir,
                     current_package->package_name);

            if(0 == stat(final, &st) && S_ISDIR(st.st_mode)) {
                log_info("Skipping %s; already exists", current_package->package_name);
            } else {
                log_info("Downloading %s", current_package->package_name);
                if(0 == stat(down, &st) && S_ISREG(st.st_mode)) {
                    log_info("  Found pre-deployed copy");
                } else {
                    snprintf(url,
                             PATH_MAX,
                             download_url_format,
                             current_package->package_name);
                    unlink(downtemp); /* ignore error */
                    if(!download(url, downtemp, proxy)) {
                        log_error("  Download failed from %s", url);
                        unlink(downtemp);
                        goto error;
                    }

                    log_info("  Download complete.");
                    if(0 != rename(downtemp, down)) {
                        log_error("  Failed to rename %s to %s: %s",
                                  downtemp, down, strerror(errno));
                        unlink(downtemp);
                        goto error;
                    }
                }

                /* untar */
                log_info("  Extracting %s", current_package->package_name);
                if(!extract_package(down, package_temp_dir)) {
                    goto error;
                }
                n++;

                /* remove original download */
                unlink(down); /* ignore error */

                /* rename extracted copy */
                if(0 != rename(temp, final)) {
                    log_error("  Failed to rename %s to %s: %s",
                              temp,
                              final,
                              strerror(errno));
                    goto error;
                }
            }
        }
    }

    result = 1;
 error:
    log_info("Extracted %d package%s.", n, n == 1 ? "" : "s");
    return result;
}

static int stow_package(const char *epkg,
                        const char *package,
                        const char *source_dir,
                        const char *target_dir)
{
    int exit_code;

    exit_code =
        run_command(epkg,
                    "-i",     /* install mode */
                    "-a",     /* absolute symlinks */
                    "-p",     /* don't perform pre-requisite checking */
                    "-R",     /* don't run package scripts */
                    "-S",     /* disable versioning */
                    "-l",     /* disable logging */
                    "-q",     /* quiet mode */
                    "-s", source_dir,
                    "-t", target_dir,
                    package,
                    NULL      /* no more args */
         );
    if(0 != exit_code) {
        log_error("Failed to link package %s", package);
        return 0;
    }
    return 1;
}

static int find_epkg_path(const package_spec_t *package_list,
                          const char *package_groups[],
                          const char *source_dir,
                          char *epkg_path)
{
    const package_spec_t *current_package;
    int g, n;
    int found = 0;

    for(current_package = package_list;
        current_package;
        current_package = current_package->next) {
        for(g = 0; package_groups[g] != NULL; g++) {
            if(!strcmp(package_groups[g], (char *)current_package->group)) {
                if(strstr((char *)current_package->package_name, "epkg-") ==
                   (char *)current_package->package_name)
                {
                    n = snprintf(epkg_path,
                                 PATH_MAX,
                                 "%s/%s/bin/epkg",
                                 source_dir,
                                 current_package->package_name);
                    if(n == PATH_MAX)
                        return 0;
                    found = 1;
                }
                break;
            }
        }
    }
    return found;
}

int create_package_tree(const package_spec_t *package_list,
                        const char *package_groups[],
                        const char *source_dir,
                        const char *target_dir)
{
    const package_spec_t *current_package;
    int g, in_group;
    char epkg_path[PATH_MAX];
    int n = 0;

    /* find epkg path */
    if(!find_epkg_path(package_list, package_groups, source_dir, epkg_path)) {
        log_error("Cannot locate epkg in %s; did you include it in your package list?", source_dir);
        return 0;
    }

    /* link packages */
    for(current_package = package_list;
        current_package;
        current_package = current_package->next) {

        for(g = 0, in_group = 0; package_groups[g] != NULL && !in_group; g++) {
            if(!strcmp(package_groups[g], (char *)current_package->group)) {
                in_group = 1;
            }
        }

        if(in_group) {
            log_info("Linking (%s): %s",
                     current_package->group,
                     current_package->package_name);

            if(!stow_package(epkg_path,
                             (char *)current_package->package_name,
                             source_dir,
                             target_dir))
            {
                return 0;
            }
            n++;
        }
    }

    log_info("Linked %d package%s.", n, n == 1 ? "" : "s");
    return 1;
}

int clean_previous_package_trees(const char *package_target_dir,
                                 const char *package_link_dir,
                                 const char *previous_package_link_dir)
{
    int result = 0;
    DIR *dp = NULL;
    struct dirent *entry;
    struct stat st;
    char full_pathname[PATH_MAX];

    if(!(dp = opendir(package_target_dir))) {
       log_error("Failed to open hostclass symlink tree directory %s: %s",
                 package_target_dir,
                 strerror(errno));
       goto error;
    }

    while(NULL != (entry = readdir(dp))) {

        if( PATH_MAX == snprintf(full_pathname, PATH_MAX, "%s/%s", package_target_dir, entry->d_name) ) {
            log_error("Hostclass symlink tree directory name is too long for buffer");
            goto error;
        }

        if( 0 == lstat(full_pathname, &st) &&
            S_ISDIR(st.st_mode) &&
            entry->d_name[0] != '.' &&
            (0 != strcmp(full_pathname, package_link_dir)) )
        {
            if(0 == strcmp(full_pathname, previous_package_link_dir)) {
                log_info("  Saving %s", entry->d_name);
            } else {
                log_info("  Removing %s", entry->d_name);
                if(!rmrf(full_pathname)) {
                    log_info("    Cannot remove directory %s; ignoring error", full_pathname);
                }
            }
        }
    }

    result = 1;
 error:
    if(dp)
        closedir(dp);
    return result;
}

int clean_previous_packages(const package_spec_t *package_list,
                            const char *package_stow_dir)
{
    const package_spec_t *cp;
    int included = 0;
    DIR *existing_packages;
    struct dirent *package;
    char full_pathname[PATH_MAX];

    existing_packages = opendir(package_stow_dir);
    if (existing_packages != NULL) {

        while(NULL != (package = readdir(existing_packages))) {
           included = 0;
           for(cp = package_list; cp; cp = cp->next) {
               if(0 == strcmp(package->d_name, (char *)cp->package_name) &&
                   package->d_name[0] != '.')
               {
                   included = 1;
                   break;
               }
           }

           if(!included && package->d_name[0] != '.') {
               snprintf(full_pathname, PATH_MAX, "%s/%s", package_stow_dir, package->d_name);
               log_info("    Removing package %s", package->d_name);
               if(!rmrf(full_pathname)) {
                   log_info("    Cannot remove directory %s; ignoring error",
                       full_pathname);
               }
           }
        }
    }

    return 0;
}
