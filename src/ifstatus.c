/* $Id: ifstatus.c 1.3 Wed, 13 Nov 2002 22:37:54 +0100 lennart $ */

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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <libdaemon/dlog.h>

#include "interface.h"
#include "svn-revision.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

int interface_auto_up = 0, interface_do_message = 0;

int verbose = 0;
char *interface = NULL;

int handle(char *iface) {
    int fd, r = 0;
    interface_status_t s;
    
    if ((fd = interface_open(iface)) < 0)
        return -1;
    
    if (verbose > 0) {
        printf("%s:\n", iface);
        
        if ((s = interface_detect_beat_ethtool(fd, iface)) == IFSTATUS_ERR)
            printf("    SIOCETHTOOL failed (%s)\n", strerror(errno));
        else
            printf("    SIOCETHTOOL: %s\n", s == IFSTATUS_UP ? "link beat detected" : "unplugged");
        
        if ((s = interface_detect_beat_mii(fd, iface)) == IFSTATUS_ERR)
            printf("    SIOCGMIIPHY failed (%s)\n", strerror(errno));
        else
            printf("    SIOCGMIIPHY: %s\n", s == IFSTATUS_UP ? "link beat detected" : "unplugged");
        
        if ((s = interface_detect_beat_priv(fd, iface)) == IFSTATUS_ERR)
            printf("    SIOCDEVPRIVATE failed (%s)\n", strerror(errno));
        else
            printf("    SIOCDEVPRIVATE: %s\n", s == IFSTATUS_UP ? "link beat detected" : "unplugged");

        if ((s = interface_detect_beat_wlan(fd, iface)) == IFSTATUS_ERR)
            printf("    Wireless failed.\n");
        else
            printf("    Wireless: %s\n", s == IFSTATUS_UP ? "link beat detected" : "unplugged");

    } else {
        
        if ((s = interface_detect_beat_mii(fd, iface)) == IFSTATUS_ERR)
	    if ((s = interface_detect_beat_ethtool(fd, iface)) == IFSTATUS_ERR)
                if ((s = interface_detect_beat_wlan(fd, iface)) == IFSTATUS_ERR)
                    s = interface_detect_beat_priv(fd, iface);

        switch(s) {
            case IFSTATUS_UP:
                if (!verbose)
                    printf("%s: link beat detected\n", iface);
                r = 1;
                break;
                
            case IFSTATUS_DOWN:
                if (!verbose)
                    printf("%s: unplugged\n", iface);
                r = 2;
                break;

            default:
                if (!verbose)
                    printf("%s: not supported\n", iface);
                
                r = -1;
                break;
        }
    }
    
            
    close(fd);
    return r;
}

void usage(char *p) {
    if (strrchr(p, '/'))
        p = strchr(p, '/')+1;

    printf("%s [options] [INTERFACE]\n"
           "   -a --auto            Enable interface automatically (%s)\n"
           "   -q --quiet           Quiet behaviour (%s)\n"
           "   -v --verbose         Enable verbosity (%s)\n"
           "   -h --help            Show this help\n"
           "   -V --version         Show version number\n",
           p, interface_auto_up ? "on" : "off", verbose < 0 ? "on" : "off", verbose > 0 ? "on" : "off");
}


void parse(int argc, char *argv[]) {
    static struct option long_options[] = {
        {"auto",        no_argument, 0, 'a'},
        {"quiet",       no_argument, 0, 'q'},
        {"verbose",     no_argument, 0, 'v'},
        {"help",        no_argument, 0, 'h'},
        {"version",     no_argument, 0, 'V'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    int help = 0;
    
    for (;;) {
        int c; 
       
        if ((c = getopt_long(argc, argv, "avhqV", long_options, &option_index)) < 0)
            break;

        switch (c) {
            case 'a' :
                interface_auto_up = !interface_auto_up;
                break;
            case 'v':
                verbose++; 
                break;
            case 'q':
                verbose--; 
                break;
            case 'h':
                help = 1;
                break;
            case 'V':
#ifdef SVN_REVISION
		 printf("ifstatus "VERSION" (SVN: "SVN_REVISION")\n");
#else
		 printf("ifstatus "VERSION"\n");
#endif
                exit(0);
            default:
                fprintf(stderr, "Unknown parameter.\n");
                exit(1);

        }
    }

    if (help) {
        usage(argv[0]);
        exit(0);
    }

    if (optind < argc)
        interface = argv[optind];
}


int main(int argc, char *argv[]) {
    parse(argc, argv);

    if (interface) {
        int r;

        if ((r = handle(interface)) < 0) {
            if (verbose == 0)
                fprintf(stderr, "Failure (%s)\n", strerror(errno));
            return 1;
        }

        return r+1;
        
    } else {
        struct ifconf ifconf;
        struct ifreq *ifr;
        int m, n, s, fd;

        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            fprintf(stderr, "socket(): %s\n", strerror(errno));
            return 1;
        }

        s = sizeof(struct ifreq)*5;
        for (;;) {
            ifr = malloc(s);

            ifconf.ifc_len = s;
            ifconf.ifc_req = ifr;
            
            if (ioctl(fd, SIOCGIFCONF, &ifconf) < 0) {
                fprintf(stderr, "SIOCGIFCONF: %s\n", strerror(errno));
                free(ifr);
                close(fd);
                return 1;
            }
                
            
            if (ifconf.ifc_len != s)
                break;
                
            free(ifr);
            s *= 2;
        }

        close(fd);
        
        m = ifconf.ifc_len/sizeof(struct ifreq);
        for (n = 0; n < m; n++)
            if (strcmp(ifconf.ifc_req[n].ifr_name, "lo"))
                handle(ifconf.ifc_req[n].ifr_name);

        free(ifr);
    }
    
    return 0;
}