/* $Id: interface.c 1.4 Thu, 07 Nov 2002 22:27:37 +0100 lennart $ */

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

#include <linux/sockios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "ethtool-local.h"
#include "interface.h"

#include <libdaemon/dlog.h>

int interface_open(char *iface) {
    return socket(AF_INET, SOCK_DGRAM, 0);
}

void interface_up(int fd, char *iface) {
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name)-1);
    
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        if (interface_do_message)
            daemon_log(LOG_WARNING, "Warning: Could not get interface flags.");
        
        return;
    }

    if ((ifr.ifr_flags & IFF_UP) == IFF_UP)
        return;
    
    if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
        if (interface_do_message)
            daemon_log(LOG_WARNING, "Warning: Could not get interface address.");
    } else if (ifr.ifr_addr.sa_family != AF_INET) {
        if (interface_do_message)
            daemon_log(LOG_WARNING, "Warning: The interface is not IP-based.");
    } else {
        ((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr.s_addr = INADDR_ANY;
        if (ioctl(fd, SIOCSIFADDR, &ifr) < 0) {
            if (interface_do_message)
                daemon_log(LOG_WARNING, "Warning: Could not set interface address.");
        }
    }


    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        if (interface_do_message)
            daemon_log(LOG_WARNING, "Warning: Could not get interface flags.");
        
        return;
    }
    
    ifr.ifr_flags |= IFF_UP;
    
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0)
        if (interface_do_message)
            daemon_log(LOG_WARNING, "Warning: Could not set interface flags.");
}

interface_status_t interface_detect_beat_mii(int fd, char *iface) {
    struct ifreq ifr;
    
    if (interface_auto_up)
        interface_up(fd, iface);
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name)-1);

    if (ioctl(fd, SIOCGMIIPHY, &ifr) == -1) {
        if (interface_do_message)
            daemon_log(LOG_ERR, "SIOCGMIIPHY failed: %s", strerror(errno));
        
        return IFSTATUS_ERR;
    }

    ((unsigned short*) &ifr.ifr_data)[1] = 1;

    if (ioctl(fd, SIOCGMIIREG, &ifr) == -1) {
        if (interface_do_message)
            daemon_log(LOG_ERR, "SIOCGMIIREG failed: %s", strerror(errno));
        
        return IFSTATUS_ERR;
    }

    return (((unsigned short*) &ifr.ifr_data)[3] & 0x0016) == 0x0004 ? IFSTATUS_UP : IFSTATUS_DOWN;
}

interface_status_t interface_detect_beat_priv(int fd, char *iface) {
    struct ifreq ifr;
    
    if (interface_auto_up)
        interface_up(fd, iface);
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name)-1);

    if (ioctl(fd, SIOCDEVPRIVATE, &ifr) == -1) {
        if (interface_do_message)
            daemon_log(LOG_ERR, "SIOCDEVPRIVATE failed: %s", strerror(errno));
        
        return IFSTATUS_ERR;
    }

    ((unsigned short*) &ifr.ifr_data)[1] = 1;

    if (ioctl(fd, SIOCDEVPRIVATE+1, &ifr) == -1) {
        if (interface_do_message)
            daemon_log(LOG_ERR, "SIOCDEVPRIVATE+1 failed: %s", strerror(errno));
        
        return IFSTATUS_ERR;
    }

    return (((unsigned short*) &ifr.ifr_data)[3] & 0x0016) == 0x0004 ? IFSTATUS_UP : IFSTATUS_DOWN;
}

interface_status_t interface_detect_beat_ethtool(int fd, char *iface) {

    struct ifreq ifr;
    struct ethtool_value edata;

    if (interface_auto_up)
        interface_up(fd, iface);
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name)-1);

    edata.cmd = ETHTOOL_GLINK;
    ifr.ifr_data = (caddr_t) &edata;

    if (ioctl(fd, SIOCETHTOOL, &ifr) == -1) {
        if (interface_do_message)
            daemon_log(LOG_ERR, "ETHTOOL_GLINK failed: %s", strerror(errno));
        
        return IFSTATUS_ERR;
    }

    return edata.data ? IFSTATUS_UP : IFSTATUS_DOWN;
}

interface_status_t interface_detect_beat_wlan(int fd, char *iface) {
    FILE *f;
    char buf[256];
    char *bp;
    int l;
    
    if (interface_auto_up)
        interface_up(fd, iface);
    
    l = strlen(iface);
    
    if (!(f = fopen("/proc/net/wireless", "r"))) {
        if (interface_do_message)
            daemon_log(LOG_WARNING, "Failed to open /proc/net/wireless: %s",strerror(errno));
        
        return IFSTATUS_ERR;
    }
    
    while (fgets(buf, sizeof(buf)-1, f)) {
        bp = buf;

        while (*bp && isspace(*bp))
            bp++;
        
        if(!strncmp(bp, iface, l) && bp[l]==':') {

            /*skip device name */
            if (!(bp = strchr(bp,' ')))
                break;

            bp++;
            
            /*skip status*/
            if (!(bp = strchr(bp,' ')))
                break;

            fclose(f);
            
            if (atoi(bp) > 0) 
                return IFSTATUS_UP;
            else
                return IFSTATUS_DOWN;
        };
    }
    
    if (interface_do_message)
        daemon_log(LOG_ERR, "Failed to find wireless interface %s\n", iface);
    
    fclose(f);
    
    return IFSTATUS_ERR;
}

