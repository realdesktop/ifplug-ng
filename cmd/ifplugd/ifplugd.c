/* -*- Mode: C; tab-width: 4; indent-tabs-mode: f; c-basic-offset: 4 -*- */
/* $Id$ */

/*
 * This file is part of ifplugd.
 *
 * ifplugd is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ifplugd is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ifplugd; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <getopt.h>
#include <stdarg.h>
#include <syslog.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>

#include <rbus.h>

#include <libdaemon/dlog.h>
#include <libdaemon/dpid.h>
#include <libdaemon/dsignal.h>
#include <libdaemon/dfork.h>

#include "ethtool-local.h"
#include "interface.h"
#include "nlapi.h"
#include "ifmonitor.h"

#define VARRUN "/var/run"
#define IFPLUGD_ENV_PREVIOUS "IFPLUGD_PREVIOUS"
#define IFPLUGD_ENV_CURRENT "IFPLUGD_CURRENT"

struct interface_state {
    char name[10];
    interface_status_t status;
    int status_time;

    interface_status_t (*detect)(int, char*);

    struct interface_state* next;
};

struct interface_state *interface;

int polltime = 1,
    delay_up = 0,
    delay_down = 5;

int daemonize = 1,
    wait_on_fork = 0,
    wait_on_kill = 0,
    use_syslog = 1,
    ignore_retval = 0;

//int disabled = 0;

const char *pid_file_proc() {
    static char fn[PATH_MAX];
    snprintf(fn, sizeof(fn), "%s/ifplugd.pid", VARRUN);
    return fn;
}

#define detect_f(fname) \
    iface->status = interface_detect_beat_##fname(fd, iface->name); \
    if(iface->status != IFSTATUS_ERR) { \
        iface->detect = interface_detect_beat_##fname; \
        return; \
    }

void detect_beat(int fd, struct interface_state *iface) {
    interface_status_t status = iface->status;

    if(iface->detect) {
        iface->status = iface->detect(fd, iface->name);

        if(status != iface->status)
            iface->status_time = 0;

        return ;
    }


    detect_f(ethtool);
    detect_f(mii);
    detect_f(wlan);
    detect_f(iff);

}

char *strstatus(interface_status_t s) {
    switch(s) {
        case IFSTATUS_UP: return "up";
        case IFSTATUS_DOWN: return "down";
        case IFSTATUS_ERR: return "error";
        default: return "disabled";
    }
}

static inline void add_interface(const char *ifname) {
    struct interface_state *iface;

    iface = malloc(sizeof(struct interface_state));

    strcpy(iface->name, ifname);
    iface->next = NULL;
    iface->status_time = -1;

    if(interface)
        interface->next = iface;
    else
        interface = iface;

}


int ifmonitor_cb(int b, int index, unsigned short type, const char *name) {

    if (!name)
        return 0;

    struct interface_state *iface, *prev = NULL;

    for(iface = interface; iface; iface = iface->next) {
        if(strcmp(iface->name, name))
            continue;

        if(b) {
            goto exit;
        } else {
            if(prev)
                prev = iface->next;

            free(iface);

            goto exit;

        }
    }

    if(!b)
        goto exit;

    add_interface(name);

    daemon_log(LOG_INFO, "added interface %s\n", iface->name);

exit:

    return 0;
}

void discover() {
    add_interface("wlan0");
}

void status_change(struct interface_state *iface) {

    if(iface->status_time==-1)
        return;

    iface->status_time++;

    int limit;
    if(iface->status == IFSTATUS_UP)
        limit = delay_up;
    else
        limit = delay_down;

    if(iface->status_time < limit)
        return;

     daemon_log(LOG_INFO, "Link beat %s.", iface->status == IFSTATUS_DOWN ? "lost" : "detected");
     iface->status_time = -1;

}

void work(void) {
    int fd = -1;
    fd_set fds;
    int sigfd;
    static char log_ident[256];

    snprintf(log_ident, sizeof(log_ident), "ifplugd");

    daemon_log_ident = log_ident;

    daemon_log(LOG_INFO, "ifplugd "VERSION" initializing, using NETLINK device monitoring");

    if (daemon_pid_file_create() < 0) {
        daemon_log(LOG_ERR, "Could not create PID file %s.", daemon_pid_file_proc());
        goto finish;
    }

    if (daemon_signal_init(SIGINT, SIGTERM, SIGQUIT, SIGHUP, SIGCHLD, SIGUSR1, SIGUSR2, -1) < 0) {
        daemon_log(LOG_ERR, "Could not register signal handler: %s", strerror(errno));
        goto finish;
    }

    if ((fd = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0) {
        daemon_log(LOG_ERR, "socket(): %s", strerror(errno));
        goto finish;
    }

    discover();

    if (nlapi_open(RTMGRP_LINK) < 0)
        goto finish;

    if (ifmonitor_init(ifmonitor_cb) < 0)
        goto finish;

    FD_ZERO(&fds);
    sigfd = daemon_signal_fd();
    FD_SET(sigfd, &fds);

    FD_SET(nlapi_fd, &fds);

    struct rbus_root *rbus;
    rbus = rbus_init("unix!/tmp/ifplugd.9p");

    for (;;) {
        struct interface_state *iface;

        fd_set qfds = fds;
        struct timeval tv;

        IxpConn *c;
        for(c = rbus->srv->conn; c; c = c->next) {
            if(c->read) {
                FD_SET(c->fd, &qfds);
            }
        }

        tv.tv_sec = polltime;
        tv.tv_usec = 0;
        
        if (select(FD_SETSIZE, &qfds, NULL, NULL, &tv) < 0) {
            if (errno == EINTR)
                continue;

            daemon_log(LOG_ERR, "select(): %s", strerror(errno));
            goto finish;
        }

        //daemon_log(LOG_INFO, "select()");
        
        for(c = rbus->srv->conn; c; c = c->next) {
            if(c->read && FD_ISSET(c->fd, &qfds)) {
                c->read(c);
            }
        }
        
        if (FD_ISSET(nlapi_fd, &qfds)) {
            if (nlapi_work(0) < 0)
                goto finish;
        }

        for(iface = interface; iface; iface=iface->next) {
            detect_beat(fd, iface);
            status_change(iface); 
        }

        if (FD_ISSET(sigfd, &qfds)) {
            int sig;

            if ((sig = daemon_signal_next()) < 0) {
                daemon_log(LOG_ERR, "daemon_signal_next(): %s", strerror(errno));
                goto finish;
            }

            switch (sig) {

                case SIGINT:
                case SIGTERM:
                    goto cleanup;
                    
                case SIGQUIT:
                    goto finish;
                    
                case SIGCHLD:
                    break;

                case SIGHUP:
                    break;
                    
                default:
                    daemon_log(LOG_INFO, "Ignoring unknown signal %s", strsignal(sig));
                    break;
            }
        }

    }

cleanup:
 
finish:

    if (fd >= 0)
        close(fd);

    nlapi_close();
    
    daemon_pid_file_remove();
    daemon_signal_done();
    
    daemon_log(LOG_INFO, "Exiting.");
}

void usage(char *p) {
    
    if (strrchr(p, '/'))
        p = strchr(p, '/')+1;

    printf("%s -- Network Interface Plug Detection Daemon\n\n"
           "Usage: %s [options]\n\n"
           "Options:\n"
           "   -n --no-daemon            Do not daemonize (for debugging) (%s)\n"
           "   -s --no-syslog            Do not use syslog, use stderr instead (for debugging) (%s)\n"
           "   -t --poll-time=SECS       Specify poll time in seconds (%i)\n"
           "   -u --delay-up=SECS        Specify delay for configuring interface (%i)\n"
           "   -d --delay-down=SECS      Specify delay for deconfiguring interface (%i)\n"
           "   -h --help                 Show this help\n"
           "   -v --version              Show version\n",
           p, p,
           !daemonize ? "on" : "off",
           !use_syslog ? "on" : "off",
           polltime,
           delay_up,
           delay_down
    );
}

void parse_args(int argc, char *argv[]) {
    static struct option long_options[] = {
        {"no-auto",              no_argument, 0, 'a'},
        {"no-daemon",            no_argument, 0, 'n'},
        {"no-syslog",            no_argument, 0, 's'},
        {"ignore-retval",        no_argument, 0, 'I'},
        {"poll-time",            required_argument, 0, 't'},
        {"delay-up",             required_argument, 0, 'u'},
        {"delay-down",           required_argument, 0, 'd'},
        {"no-startup",           no_argument, 0, 'p'},
        {"no-shutdown",          no_argument, 0, 'q'},
        {"help",                 no_argument, 0, 'h'},
        {"version",              no_argument, 0, 'v'},
        {"extra-arg",            required_argument, 0, 'x'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    int help = 0, _version = 0;
    
    for (;;) {
        int c;
        
        if ((c = getopt_long(argc, argv, "asni:r:t:u:d:hkbfFvm:pqwx:cISRzlMW", long_options, &option_index)) < 0)
            break;

        switch (c) {
            case 's' :
                use_syslog = !use_syslog;
                break;
            case 'n' :
                daemonize = !daemonize;
                break;
            case 't':
                polltime = atoi(optarg);
                if (polltime < 0) polltime = 0;
                break;
            case 'u':
                delay_up = atoi(optarg);
                break;
            case 'd':
                delay_down = atoi(optarg);
                break;
            case 'h':
                help = 1;
                break;
           case 'v':
                _version = 1;
                break;
            default:
                daemon_log(LOG_ERR, "Unknown parameter.");
                exit(1);
        }
    }


    if (!use_syslog)
        daemon_log_use = DAEMON_LOG_STDERR;
    

    if (help) {
        usage(argv[0]);
        exit(0);
    }

    if (_version) {

        printf("ifplugd "VERSION"\n");
        exit(0);
    }

}

static volatile int alarmed = 0;

void sigalrm() {
    alarmed = 1;
}

int main(int argc, char* argv[]) {

    daemon_pid_file_proc = pid_file_proc;

    if ((daemon_log_ident = strrchr(argv[0], '/')))
        daemon_log_ident++;
    else
        daemon_log_ident = argv[0];

    parse_args(argc, argv);

/*
    if (geteuid() != 0) {
        daemon_log(LOG_ERR, "Sorry, you need to be root to run this binary.");
        return 2;
    }
*/

    if (daemon_pid_file_is_running() >= 0) {
        daemon_log(LOG_ERR, "Sorry, there is already an instance of ifplugd running.");
        return 4;
    }
    
    if (daemonize) {
        pid_t pid;

        if ((pid = daemon_fork()) < 0)
            return 3;

        if (pid) {
            int c = 0;
            
            // Parent process

            if (c > 3)
                daemon_log(LOG_ERR, "Daemon failed with error condition #%i. See syslog for details", c);
            
            return c;
        }
    }

    // Let's go
    work();
    daemon_log(LOG_ERR, "stop");
    return 0;
}
