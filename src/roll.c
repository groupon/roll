/* roll.c - bootstrap or upgrade a Unix host with Roller
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
#ifdef HAVE_STDLIB_H
    #include <stdlib.h>
#endif
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
#ifdef HAVE_SYS_SOCKET_H
    #include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
    #include <netdb.h>
#endif
#ifdef HAVE_FCNTL_H
    #include <fcntl.h>
#endif
#include <errno.h>
#ifdef HAVE_LIMITS_H
    #include <limits.h>
#endif
#include <getopt.h>
#include <libgen.h>
#include "log.h"
#include "strlcpy.h"
#include "config_parse.h"
#include "environ.h"
#include "packages.h"
#include "mkpath.h"
#include "rmrf.h"
#include "cp.h"
#include "download.h"
#include "local_initd.h"
#include "local_profiled.h"
#include "spawn.h"

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif

#define EXE_MODE \
    (S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH)

#define BASE_URL "http://config"
#define DOWNLOAD_URL_METAFORMAT "%s/package/%%s.tar.gz"
#define HOSTCLASS_CONFIG_URL_FORMAT "%s/hostclass/%s"
#define HOST_CONFIG_URL_FORMAT "%s/host/%s"
#define PACKAGE_DIR "/packages"
#define PACKAGE_DOWNLOAD_DIR_FORMAT "%s/download"
#define PACKAGE_TEMP_DIR_FORMAT "%s/tmp"
#define PACKAGE_STOW_DIR_FORMAT "%s/encap"
#define PACKAGE_TARGET_DIR_FORMAT "%s/installed"
#define PACKAGE_TARGET_LINK "/usr/local"
#define CONFIG_DIR "/usr/local/etc"
#define CONFIGURATE "/usr/local/bin/configurate"
#define PID_FILE "/var/run/roll.pid"

#define USAGE "usage: roll [options] [hostclass.yml] [host.yml]\n"
#define FULL_USAGE USAGE \
    "  -h, --help        display this help and exit\n" \
    "  -f, --failsafe    failsafe mode, for testing\n" \
    "  -u, --baseurl     base URL for hostclass and host files, packages (default " BASE_URL ")\n" \
    "  -d, --packagedir  install packages in this directory (default " PACKAGE_DIR ")\n" \
    "  -i, --initd       write local_initd here (default " LOCAL_INITD_FILENAME ")\n" \
    "  -b, --profiled    write bash local_profiled here (default " LOCAL_PROFILED_FILENAME ")\n" \
    "  -c, --configdir   put generated config files here (default " CONFIG_DIR ")\n" \
    "  -o, --logfile     log here (default " LOG_DIR_ROOT "/" LOG_DIR_SUBDIR "/roll.{date}.log)\n" \
    "  -x, --proxy       optional HTTP Proxy specified as: proxyhost[:port]\n" \
    "  -p, --pidfile     store PID here (default " PID_FILE ")\n" \
    "  -r, --prune       delete unused packages from previous installations\n" \
/*  Don't advertise --dryrun since some steps will still do things to the system.  It's   */
/*  still useful for testing.  So it remains as a hidden feature.                         */
/*  "  -n, --dryrun      just print what would happen for some things\n" \                */

typedef struct options_t_s {
    int failsafe;
    int dryrun;
    int prune;
    char *base_url;
    char *package_dir;
    char *local_initd_file;
    char *local_profiled_file;
    char *config_dir;
    char *log_file;
    char *pid_file;
    char *hostclass_file;
    char *host_file;
    char *proxy;
} options_t;

static int parse_commandline (int argc, char *argv[], options_t *options) {
    int ch;

    static const char shortopts[] = "hfrnu:d:i:b:c:o:p:x:";
    static struct option longopts[] = {
        { "help",         no_argument,       NULL, 'h' },
        { "failsafe",     no_argument,       NULL, 'f' },
        { "dryrun",       no_argument,       NULL, 'n' },
        { "prune",        no_argument,       NULL, 'r' },
        { "baseurl",      required_argument, NULL, 'u' },
        { "packagedir",   required_argument, NULL, 'd' },
        { "initd",        required_argument, NULL, 'i' },
        { "profiled",     required_argument, NULL, 'b' },
        { "configdir",    required_argument, NULL, 'c' },
        { "logfile",      required_argument, NULL, 'o' },
        { "pidfile",      required_argument, NULL, 'p' },
        { "proxy",        required_argument, NULL, 'x' },
        { NULL,           0,                 NULL, 0   }
    };

    while((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch(ch) {
        case 'h':
            printf(FULL_USAGE);
            exit(0);
            break;
        case 'f':
            options->failsafe = 1;
            break;
        case 'n':
            options->dryrun = 1;
            break;
        case 'r':
            options->prune = 1;
            break;
        case 'u':
            options->base_url = optarg;
            break;
        case 'd':
            options->package_dir = optarg;
            break;
        case 'i':
            options->local_initd_file = optarg;
            break;
        case 'b':
            options->local_profiled_file = optarg;
            break;
        case 'c':
            options->config_dir = optarg;
            break;
        case 'o':
            options->log_file = optarg;
            break;
        case 'p':
            options->pid_file = optarg;
            break;
        case 'x':
            options->proxy = optarg;
            break;
        default:
            fprintf(stderr, USAGE);
            exit(1);
        }
    }
    argc -= optind;
    argv += optind;

    if(argc > 1) {
        options->host_file = argv[argc-1];
        options->hostclass_file = argv[argc-2];
    } else if(argc > 0) {
        options->hostclass_file = argv[argc-1];
    }

    return 1;
}

static int has_root_privileges() {
    if(geteuid() != 0) {
        fprintf(stderr, "You must have root privileges to run this command.\n");
        return 0;
    }
    return 1;
}

static int open_pid_file(int *pid_file_fd, char *pid_file) {
    char str[40];
    int n;

    *pid_file_fd = open(pid_file, O_RDWR|O_CREAT, 0640);
    if(*pid_file_fd < 0) {
        fprintf(stderr, "Cannot write pidfile to %s\n", PID_FILE);
        return 0;
    }
    if(lockf(*pid_file_fd, F_TLOCK, 0) < 0) {
        fprintf(stderr, "Only one copy of this program can be run at once.\n");
        return 0;
    }
    n = snprintf(str, 40, "%ld\n", (long)getpid());
    ftruncate(*pid_file_fd, 0);
    write(*pid_file_fd, str, n);
    return 1;
}

static void close_pid_file(int pid_file_fd, char *pid_file) {
    close(pid_file_fd);
    unlink(pid_file);
}

static int get_hostname(char *hostname) {
    struct addrinfo hints, *info, *p;
    int gai_result;

    if(0 != gethostname(hostname, HOST_NAME_MAX)) {
        return 0;
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; /* IPV4 or IPV6 */
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;
    if((gai_result = getaddrinfo(hostname, "http", &hints, &info)) != 0) {
        log_error("Unable to determine hostname: %s", gai_strerror(gai_result));
        return 0;
    }
    for(p = info; p != NULL; p = p->ai_next) {
        strncpy(hostname, p->ai_canonname, HOST_NAME_MAX);
        /* there can be multiple; we'll just take the first one */
        break;
    }
    return 1;
}

static int dir_exists(const char *path) {
    struct stat st;
    if(0 != stat(path, &st)) {
        if(ENOENT != errno) {
            log_error("%s: cannot stat", path);
            return 0;
        } else {
            return 0;
        }
    } else {
        return S_ISDIR(st.st_mode);
    }
}

int main(int argc, char *argv[]) {
    options_t options;
    int exit_code = 0;
    int pid_file_fd = -1;
    int try_failsafe = 0;
    int failsafe_mode = 0;
    int prune_packages = 0;
    int report_errors = 0;
    FILE *hostclass_file = NULL,
         *host_file = NULL;
    FILE *fp = NULL;
    host_config_t host_config;
    hostclass_config_t hostclass_config;
    struct stat st;
    char hostclass_file_tmpname[PATH_MAX],  /* "/tmp/hostclass.yml" */
         hostclass_file_name[PATH_MAX],     /* "/usr/local/etc/hostclass.yml" */
         host_file_tmpname[PATH_MAX],       /* "/tmp/host.yml" */
         host_file_name[PATH_MAX],          /* "/usr/local/etc/host.yml" */
         hostclass_config_url[PATH_MAX],
         host_config_url[PATH_MAX],
         download_url_format[PATH_MAX],
         temp_package_link_dir[PATH_MAX],
         previous_package_link_dir[PATH_MAX],
         package_download_dir[PATH_MAX],
         package_temp_dir[PATH_MAX],
         package_stow_dir[PATH_MAX],
         package_target_dir[PATH_MAX],
         package_link_dir[PATH_MAX],
         local_profiled_file_copy[PATH_MAX], *local_profiled_dir,
         pathbuf[PATH_MAX];
    char hostname[HOST_NAME_MAX];
    const char *base_groups[] = {"production", NULL};
    const char *failsafe_groups[] = {"failsafe", NULL};
    const char *download_groups[] = {"production", "failsafe", NULL};

    #define SNPRINTF_OR_ERROR(label, str, size, fmt, ...) \
        { int n = snprintf((str), (size), (fmt), __VA_ARGS__); \
          if(n == (size)) { log_error("%s is too long for buffer", (label)); \
              goto error; } }
    #define MKPATH_OR_ERROR(label, path) \
        { strlcpy(pathbuf, (path), sizeof(pathbuf)); \
          if(0 != mkpath((pathbuf)) && EEXIST != errno) { \
              log_error("Cannot make %s directory %s", (label), (pathbuf)); \
              goto error; } \
          if(0 != chmod((path), 0755)) { \
              log_error("Cannot chmod %s directory %s", (label), (path)); \
              goto error; } }
    #define RMRF_OR_ERROR(label, path) \
        { if(dir_exists(path) && !rmrf(path)) { \
              log_error("Cannot remove %s directory %s", (label), (path)); \
              goto error; } }
    #define CP_OR_ERROR(label, source, dest, dest_mode)       \
        { if(0 != cp(source, dest, dest_mode)) { \
              log_error ("Cannot copy %s to destination %s", (label), (dest)); \
              goto error; } }

    memset(&host_config, 0, sizeof(host_config_t));
    memset(&hostclass_config, 0, sizeof(hostclass_config_t));
    memset(&options, 0, sizeof(options_t));
    memset(hostclass_file_tmpname, 0, sizeof(hostclass_file_tmpname));
    memset(host_file_tmpname, 0, sizeof(host_file_tmpname));
    memset(previous_package_link_dir, 0, sizeof(previous_package_link_dir));

    options.base_url = BASE_URL;
    options.package_dir = PACKAGE_DIR;
    options.local_initd_file = LOCAL_INITD_FILENAME;
    options.local_profiled_file = LOCAL_PROFILED_FILENAME;
    options.config_dir = CONFIG_DIR;
    options.log_file = NULL;
    options.pid_file = PID_FILE;
    options.proxy = NULL;

    /* === Start ====================================================== */
    if(!parse_commandline(argc, argv, &options)) {
        goto error;
    }

    if(options.failsafe) {
        failsafe_mode = 1;
    }

    if(options.prune) {
      prune_packages = 1;
    }

    if(!(options.dryrun || has_root_privileges())) {
        goto error;
    }

    if(!open_pid_file(&pid_file_fd, options.pid_file)) {
        goto error;
    }

    if(!log_init(options.log_file, NULL)) {
        goto error;
    }

    report_errors = 1;
    log_header("Initializing", failsafe_mode);
    log_message("Roll starting with pid %ld\n", (long)getpid());
    log_message("Logging to %s\n", get_log_filename());

    if(!sanitize_environment()) {
        goto error;
    }

    if(!get_hostname(hostname)) {
        goto error;
    }
    log_message("Hostname is %s\n", hostname);

    /* === Fetch configuration ======================================== */
    log_header("Fetching config files", failsafe_mode);

    /* fetch host file, a versioned snapshot of a host file */
    if(options.host_file) {
        log_info("Using user specified host file %s", options.host_file);
        strlcpy(host_file_tmpname, options.host_file, sizeof(host_file_tmpname));
    } else {
        /* fetch the host config file */
        SNPRINTF_OR_ERROR(
            "Host configuration URL",
            host_config_url, PATH_MAX, HOST_CONFIG_URL_FORMAT,
            options.base_url, hostname
        );
        SNPRINTF_OR_ERROR(
            "Temporary host filename",
            host_file_tmpname, PATH_MAX, "/var/tmp/roll_host.%ld",
            (long)getpid()
        );
        unlink(host_file_tmpname); /* ignore error */
        log_info("Downloading host config from %s", host_config_url);
        if(!download(host_config_url, host_file_tmpname, options.proxy)) {
            goto error;
        }
        log_info("Saved host file to %s", host_file_tmpname);
    }

    host_file = fopen(host_file_tmpname, "rb");
    if(!host_file) {
        log_error("Unable to open %s: %s", host_file_tmpname, strerror(errno));
        goto error;
    }

    /* parse the host file, figure out which hostclass file to fetch */
    log_info("Parsing host config file");
    if(!parse_host_config(&host_config, host_file)) {
        goto error;
    }
    if(!host_config.hostclass_tag) {
        log_error("The host configuration does not specify a hostclass tag");
        goto error;
    }

    /* fetch hostclass file, a verisioned snapshot of a hostclass file */
    if(options.hostclass_file) {
        log_info("Using user specified hostclass file %s", options.hostclass_file);
        strlcpy(hostclass_file_tmpname, options.hostclass_file, sizeof(hostclass_file_tmpname));
    } else {
        /* fetch the hostclass config file */
        SNPRINTF_OR_ERROR(
            "Hostclass configuration URL",
            hostclass_config_url, PATH_MAX, HOSTCLASS_CONFIG_URL_FORMAT,
            options.base_url, host_config.hostclass_tag
        );
        SNPRINTF_OR_ERROR(
            "Temporary hostclass filename",
            hostclass_file_tmpname, PATH_MAX, "/var/tmp/roll_hostclass.%ld",
            (long)getpid()
        );
        unlink(hostclass_file_tmpname); /* ignore error */
        log_info("Downloading hostclass config from %s", hostclass_config_url);
        if(!download(hostclass_config_url, hostclass_file_tmpname, options.proxy)) {
            goto error;
        }
        log_info("Saved hostclass config file to %s", hostclass_file_tmpname);
    }

    log_info("Parsing hostclass config file");
    hostclass_file = fopen(hostclass_file_tmpname, "rb");
    if(!hostclass_file) {
        log_error("Unable to open %s: %s", hostclass_file_tmpname, strerror(errno));
        goto error;
    }
    if(!parse_hostclass_config(&hostclass_config, hostclass_file)) {
        goto error;
    }

    /* === Configuration ======================================== */
    log_header("Configuration", failsafe_mode);
    /* TODO verify image is defined */
    /* TODO verify current image matches desired image */

    log_message("Hostclass tag:     %s\n", host_config.hostclass_tag);
    log_message("Hardware type:     %s\n", "__TODO__");
    log_message("Current OS image:  %s\n", "__TODO__");
    log_message("Required OS image: %s\n", "__TODO__");

    /* === Download packages ========================================== */
    log_header("Downloading packages", failsafe_mode);

    SNPRINTF_OR_ERROR(
        "Package stow directory name",
        package_stow_dir, PATH_MAX, PACKAGE_STOW_DIR_FORMAT,
        options.package_dir
    );
    SNPRINTF_OR_ERROR(
        "Package download directory name",
        package_download_dir, PATH_MAX, PACKAGE_DOWNLOAD_DIR_FORMAT,
        options.package_dir
    );
    SNPRINTF_OR_ERROR(
        "Package temp directory name",
        package_temp_dir, PATH_MAX, PACKAGE_TEMP_DIR_FORMAT,
        options.package_dir
    );
    SNPRINTF_OR_ERROR(
        "Download URL format",
        download_url_format, PATH_MAX, DOWNLOAD_URL_METAFORMAT,
        options.base_url
    );
    MKPATH_OR_ERROR("package repository", package_stow_dir);
    MKPATH_OR_ERROR("package download", package_download_dir);
    MKPATH_OR_ERROR("package temp", package_temp_dir);
    if(!download_packages(hostclass_config.package_list,
                          download_groups,
                          download_url_format,
                          package_stow_dir,
                          package_download_dir,
                          package_temp_dir,
                          options.proxy))
    {
        goto error;
    }

 failsafe:

    /* === Build symlink tree ========================================= */
    log_header("Building symlink tree", failsafe_mode);
    /* TODO determine additional package groups to link from host */

    /* Figure out where to put things */
    SNPRINTF_OR_ERROR(
        "Package target directory name",
        package_target_dir, PATH_MAX, PACKAGE_TARGET_DIR_FORMAT,
        options.package_dir
    );
    SNPRINTF_OR_ERROR(
        "Hostclass symlink tree directory name",
        package_link_dir, PATH_MAX, "%s/%s%s",
        package_target_dir,
        (failsafe_mode ?
            (options.hostclass_file ? "__FAILSAFE__DEV__" : "__FAILSAFE__") :
            (options.hostclass_file ? "__DEV__" : "") ),
        host_config.hostclass_tag
    );
    SNPRINTF_OR_ERROR(
        "Hostclass temp symlink tree directory name",
        temp_package_link_dir, PATH_MAX, "%s.%ld",
        package_link_dir, (long)getpid()
    );

    RMRF_OR_ERROR("temporary package link", temp_package_link_dir);
    MKPATH_OR_ERROR("temporary package link", temp_package_link_dir);

    if(!create_package_tree(hostclass_config.package_list,
                            (failsafe_mode ? failsafe_groups : base_groups),
                            package_stow_dir,
                            temp_package_link_dir))
    {
        goto error;
    }

    /* !! Any failures from here on out will trigger failsafe mode */
    try_failsafe = 1;

    /* === Install /etc/init.d/local_initd ============================ */
    log_header("Installing local_initd script", failsafe_mode);
    if(!(fp = fopen(options.local_initd_file, "w")) ) {
        log_error("  Cannot open %s for writing.", options.local_initd_file);
        goto error;
    }
    fputs(LOCAL_INITD_SCRIPT, fp);
    fclose(fp);
    if(0 != chmod(options.local_initd_file, EXE_MODE)) {
        log_error("  Could not chmod %04o %s", EXE_MODE, options.local_initd_file);
        goto error;
    }
    log_info("Installed %s", options.local_initd_file);

    if(options.dryrun) {
        log_info("Skipping symlink creation in dry run mode");
    } else {

        /* If Gentoo/OpenRC, use textual default runlevel */
        if(dir_exists("/etc/runlevels/default")) {

            unlink(OPENRC_LOCAL_INITD_SYMLINK);    /* ignore errors */
            if(0 != symlink(LOCAL_INITD_FILENAME, OPENRC_LOCAL_INITD_SYMLINK)) {
                log_error("  Could not symlink %s to %s: %s", LOCAL_INITD_SCRIPT, OPENRC_LOCAL_INITD_SYMLINK, strerror(errno));
                goto error;
            }
            log_info("Installed Gentoo/OpenRC runlevel symlink %s", OPENRC_LOCAL_INITD_SYMLINK);

        /* Otherwise, assume SysV numeric runlevels */
        } else {

            char **s, *symlink_dests[] = LOCAL_INITD_SYMLINKS;
            for(s = symlink_dests; *s != NULL; s++) {
                unlink(*s);
                if(0 != symlink(LOCAL_INITD_FILENAME, *s)) {
                    log_error("  Could not symlink %s to %s: %s", LOCAL_INITD_SCRIPT, *s, strerror(errno));
                    goto error;
                }
                log_info("Installed SysV runlevel symlink %s", *s);
            }

        }
    }

    /* === Install /etc/profile.d/local_profiled.sh =================== */
    log_header("Installing local_profiled.sh script", failsafe_mode);
    strlcpy(local_profiled_file_copy, options.local_profiled_file, sizeof(local_profiled_file_copy));
    local_profiled_dir = dirname(local_profiled_file_copy);
    MKPATH_OR_ERROR("bash local_profiled.sh directory", local_profiled_dir);
    if(!(fp = fopen(options.local_profiled_file, "w")) ) {
        log_error("  Cannot open %s for writing.", options.local_profiled_file);
        goto error;
    }
    fputs(LOCAL_PROFILED_SCRIPT, fp);
    fclose(fp);
    log_info("Installed %s", options.local_profiled_file);

    /* === Run /etc/init.d/local_initd stop =========================== */
    log_header("Shutting down services", failsafe_mode);
    if(options.dryrun) {
        log_info("Skipping in dry run mode");
    } else {
        exit_code = run_command(options.local_initd_file, "stop", NULL);
        if(0 != exit_code) {
            log_error("  Could not run %s stop (status = %d)", options.local_initd_file, exit_code);
            goto error;
        }
    }

    /* === Move symlink tree into place =============================== */
    log_header("Moving package link tree into /usr/local", failsafe_mode);
    if(options.dryrun) {
        log_info("Skipping in dry run mode");
    } else {
        if(0 == lstat(PACKAGE_TARGET_LINK, &st)) {
            if(S_ISLNK(st.st_mode)) {
                log_message("Removing old %s symlink\n", PACKAGE_TARGET_LINK);

                /* Remember the current value for later; ignore error */
                readlink(PACKAGE_TARGET_LINK, previous_package_link_dir, PATH_MAX);

                if(0 != unlink(PACKAGE_TARGET_LINK)) {
                    log_error("Cannot unlink %s: %s", PACKAGE_TARGET_LINK, strerror(errno));
                    goto error;
                }
            } else {
                log_error("%s is expected to be missing or a symlink", PACKAGE_TARGET_LINK);
                goto error;
            }
        } else {
            if(ENOENT != errno) {
                log_error("%s: cannot stat", PACKAGE_TARGET_LINK);
                goto error;
            }
        }

        if(dir_exists(package_link_dir)) {
            log_message("Removing old link tree\n");
            RMRF_OR_ERROR("old package link", package_link_dir);
        }

        log_message("Moving link tree to %s\n", package_link_dir);
        if(0 != rename(temp_package_link_dir, package_link_dir)) {
            log_error("Cannot move temp symlink tree from %s to %s: %s", temp_package_link_dir, package_link_dir, strerror(errno));
            goto error;
        }

        log_message("Creating symlink from %s to %s\n", PACKAGE_TARGET_LINK, package_link_dir);
        if(0 != symlink(package_link_dir, PACKAGE_TARGET_LINK)) {
            log_error("Cannot create symlink from %s to %s: %s", PACKAGE_TARGET_LINK, package_link_dir, strerror(errno));
            goto error;
        }
    }

    /* === Copy hostclass file and host file to config_dir ====== */
    SNPRINTF_OR_ERROR(
        "hostclass configuration file",
        hostclass_file_name, PATH_MAX, "%s/hostclass.yml",
        options.config_dir
    );
    CP_OR_ERROR("hostclass configuration file",
        hostclass_file_tmpname,
        hostclass_file_name,
        0644
    );

    SNPRINTF_OR_ERROR(
        "host configuration file",
        host_file_name, PATH_MAX, "%s/host.yml",
        options.config_dir
    );
    CP_OR_ERROR("host configuration file",
        host_file_tmpname,
        host_file_name,
        0644
    );

    /* === Run configuration scripts ================================== */
    log_header("Processing package configuration scripts", failsafe_mode);

    MKPATH_OR_ERROR("config output directory", options.config_dir);

    exit_code = run_command(CONFIGURATE,
                            "--template-outdir", options.config_dir,
                            hostclass_file_tmpname,
                            host_file_tmpname,
                            NULL
    );
    if(0 != exit_code) {
        log_error("  Exit code from %s is %d", CONFIGURATE, exit_code);
        goto error;
    }

    /* === Run /etc/init.d/local_initd start ========================== */
    log_header("Starting services", failsafe_mode);
    if(options.dryrun) {
        log_info("Skipping in dry run mode");
    } else {
        exit_code = run_command(options.local_initd_file, "start", NULL);
        if(0 != exit_code) {
            log_error("  Could not run %s start", options.local_initd_file);
            goto error;
        }
    }

    /* === Cleanup ==================================================== */
    log_header("Cleanup", failsafe_mode);

    if(!failsafe_mode) {
        log_info("Removing package target directories from prior installations.");
        /* ignore errors */
        clean_previous_package_trees(package_target_dir,
                                     options.dryrun ? temp_package_link_dir : package_link_dir,
                                     previous_package_link_dir);
        if(options.dryrun) {
          log_info("Skipping package prune in dry run mode");
        }
        else if (prune_packages) {
          log_info("Removing unused packages from prior installations.");
          clean_previous_packages(hostclass_config.package_list,
                                  package_stow_dir);
        }
    } else {
        log_info("Not removing package target directories from prior installations in failsafe mode.");
    }

    /* TODO reboot if necessary */

    goto done;
 error:
    if(!failsafe_mode && try_failsafe) {
        if(hostclass_config.has_failsafe) {
            failsafe_mode = 1;
            log_message("\n!!! Falling back to failsafe configuration...\n");
            goto failsafe;
        } else {
            log_message("\n!!! Tried to enter failsafe mode, but the hostclass config is\n");
            log_message("!!! missing a failsafe mode definition.\n");
        }
    }
    exit_code = 1;
 done:
    if(host_file)
        fclose(host_file);
    host_file = NULL;
    if(host_file_tmpname[0] && !options.host_file)
        unlink(host_file_tmpname);

    if(hostclass_file)
        fclose(hostclass_file);
    hostclass_file = NULL;
    if(hostclass_file_tmpname[0] && !options.hostclass_file)
        unlink(hostclass_file_tmpname);
    free_hostclass_config(&hostclass_config);

    if(report_errors) {
        if(failsafe_mode) {
            if(exit_code != 0) {
                log_header("Failsafe roll failed", failsafe_mode);
                log_message("!!! An error occurred during both normal and failsafe mode.\n\n");
            } else {
                log_header("Failsafe roll succeeded", failsafe_mode);
                log_message("!!! An error occurred during normal roll.\n");
                log_message("!!! Failsafe mode completed successfully.\n\n");
            }
        } else {
            if(exit_code != 0) {
                log_header("Roll failed; failsafe mode not attempted", failsafe_mode);
                log_message("!!! An error occurred during roll.\n\n");
            } else {
                log_header("Roll succeeded", failsafe_mode);
                log_message("!!! Done.\n\n");
            }
        }
    }

    log_close();
    if(pid_file_fd >= 0)
        close_pid_file(pid_file_fd, options.pid_file);
    return exit_code;

    #undef SNPRINTF_OR_ERROR
    #undef MKPATH_OR_ERROR
    #undef RMRF_OR_ERROR
}
