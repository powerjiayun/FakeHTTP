/*
 * ipv4ipt.c - FakeHTTP: https://github.com/MikeWang000000/FakeHTTP
 *
 * Copyright (C) 2025  MikeWang000000
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _GNU_SOURCE
#include "ipv4ipt.h"

#include <inttypes.h>
#include <stdlib.h>
#include <net/if.h>

#include "globvar.h"
#include "logging.h"
#include "process.h"

static int ipt4_iface_setup(void)
{
    char iface_str[IFNAMSIZ];
    size_t i;
    int res;
    char *ipt_alliface_src_cmd[] = {"iptables", "-w",         "-t",
                                    "mangle",   "-A",         "FAKEHTTP_S",
                                    "-j",       "FAKEHTTP_R", NULL};

    char *ipt_alliface_dst_cmd[] = {"iptables", "-w",         "-t",
                                    "mangle",   "-A",         "FAKEHTTP_D",
                                    "-j",       "FAKEHTTP_R", NULL};

    char *ipt_iface_src_cmd[] = {"iptables", "-w",         "-t", "mangle",
                                 "-A",       "FAKEHTTP_S", "-i", iface_str,
                                 "-j",       "FAKEHTTP_R", NULL};

    char *ipt_iface_dst_cmd[] = {"iptables", "-w",         "-t", "mangle",
                                 "-A",       "FAKEHTTP_D", "-o", iface_str,
                                 "-j",       "FAKEHTTP_R", NULL};

    if (g_ctx.alliface) {
        res = fh_execute_command(ipt_alliface_src_cmd, 0, NULL);
        if (res < 0) {
            E(T(fh_execute_command));
            return -1;
        }
        res = fh_execute_command(ipt_alliface_dst_cmd, 0, NULL);
        if (res < 0) {
            E(T(fh_execute_command));
            return -1;
        }
        return 0;
    }

    for (i = 0; g_ctx.iface[i]; i++) {
        res = snprintf(iface_str, sizeof(iface_str), "%s", g_ctx.iface[i]);
        if (res < 0 || (size_t) res >= sizeof(iface_str)) {
            E("ERROR: snprintf(): %s", "failure");
            return -1;
        }

        res = fh_execute_command(ipt_iface_src_cmd, 0, NULL);
        if (res < 0) {
            E(T(fh_execute_command));
            return -1;
        }

        res = fh_execute_command(ipt_iface_dst_cmd, 0, NULL);
        if (res < 0) {
            E(T(fh_execute_command));
            return -1;
        }
    }
    return 0;
}


int fh_ipt4_setup(void)
{
    char xmark_str[64], nfqnum_str[32];
    size_t i, ipt_cmds_cnt, ipt_opt_cmds_cnt;
    int res;
    char *ipt_cmds[][32] = {
        {"iptables", "-w", "-t", "mangle", "-N", "FAKEHTTP_S", NULL},

        {"iptables", "-w", "-t", "mangle", "-N", "FAKEHTTP_D", NULL},

        {"iptables", "-w", "-t", "mangle", "-I", "PREROUTING", "-j",
         "FAKEHTTP_S", NULL},

        {"iptables", "-w", "-t", "mangle", "-I", "POSTROUTING", "-j",
         "FAKEHTTP_D", NULL},

        {"iptables", "-w", "-t", "mangle", "-N", "FAKEHTTP_R", NULL},

        /*
            exclude local IPs (from source)
        */
        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_S", "-s",
         "0.0.0.0/8", "-j", "RETURN", NULL},

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_S", "-s",
         "10.0.0.0/8", "-j", "RETURN", NULL},

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_S", "-s",
         "100.64.0.0/10", "-j", "RETURN", NULL},

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_S", "-s",
         "127.0.0.0/8", "-j", "RETURN", NULL},

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_S", "-s",
         "169.254.0.0/16", "-j", "RETURN", NULL},

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_S", "-s",
         "172.16.0.0/12", "-j", "RETURN", NULL},

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_S", "-s",
         "192.168.0.0/16", "-j", "RETURN", NULL},

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_S", "-s",
         "224.0.0.0/3", "-j", "RETURN", NULL},

        /*
            exclude local IPs (to destination)
        */
        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_D", "-d",
         "0.0.0.0/8", "-j", "RETURN", NULL},

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_D", "-d",
         "10.0.0.0/8", "-j", "RETURN", NULL},

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_D", "-d",
         "100.64.0.0/10", "-j", "RETURN", NULL},

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_D", "-d",
         "127.0.0.0/8", "-j", "RETURN", NULL},

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_D", "-d",
         "169.254.0.0/16", "-j", "RETURN", NULL},

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_D", "-d",
         "172.16.0.0/12", "-j", "RETURN", NULL},

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_D", "-d",
         "192.168.0.0/16", "-j", "RETURN", NULL},

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_D", "-d",
         "224.0.0.0/3", "-j", "RETURN", NULL},

        /*
            exclude marked packets
        */

        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_R", "-m", "mark",
         "--mark", xmark_str, "-j", "RETURN", NULL},

        /*
            send to nfqueue
        */
        {"iptables", "-w", "-t", "mangle", "-A", "FAKEHTTP_R", "-p", "tcp",
         "--tcp-flags", "SYN,FIN,RST", "SYN", "-j", "NFQUEUE",
         "--queue-bypass", "--queue-num", nfqnum_str, NULL}};

    char *ipt_opt_cmds[][32] = {
        /*
            Also enqueue some of the early ACK packets to ensure the packet
            order. This rule is optional. We do not verify its execution
            result.
        */
        {"iptables",    "-w",
         "-t",          "mangle",
         "-A",          "FAKEHTTP_R",
         "-p",          "tcp",
         "--tcp-flags", "SYN,ACK,FIN,RST",
         "ACK",         "-m",
         "connbytes",   "--connbytes",
         "2:4",         "--connbytes-dir",
         "both",        "--connbytes-mode",
         "packets",     "-j",
         "NFQUEUE",     "--queue-bypass",
         "--queue-num", nfqnum_str,
         NULL}};

    ipt_cmds_cnt = sizeof(ipt_cmds) / sizeof(*ipt_cmds);
    ipt_opt_cmds_cnt = sizeof(ipt_opt_cmds) / sizeof(*ipt_opt_cmds);

    res = snprintf(xmark_str, sizeof(xmark_str), "%" PRIu32 "/%" PRIu32,
                   g_ctx.fwmark, g_ctx.fwmask);
    if (res < 0 || (size_t) res >= sizeof(xmark_str)) {
        E("ERROR: snprintf(): %s", "failure");
        return -1;
    }

    res = snprintf(nfqnum_str, sizeof(nfqnum_str), "%" PRIu32, g_ctx.nfqnum);
    if (res < 0 || (size_t) res >= sizeof(nfqnum_str)) {
        E("ERROR: snprintf(): %s", "failure");
        return -1;
    }

    fh_ipt4_cleanup();

    for (i = 0; i < ipt_cmds_cnt; i++) {
        res = fh_execute_command(ipt_cmds[i], 0, NULL);
        if (res < 0) {
            E(T(fh_execute_command));
            return -1;
        }
    }

    for (i = 0; i < ipt_opt_cmds_cnt; i++) {
        fh_execute_command(ipt_opt_cmds[i], 1, NULL);
    }

    res = ipt4_iface_setup();
    if (res < 0) {
        E(T(ipt4_iface_setup));
        return -1;
    }

    return 0;
}


void fh_ipt4_cleanup(void)
{
    size_t i, cnt;
    char *ipt_cmds[][32] = {
        {"iptables", "-w", "-t", "mangle", "-F", "FAKEHTTP_R", NULL},

        {"iptables", "-w", "-t", "mangle", "-F", "FAKEHTTP_S", NULL},

        {"iptables", "-w", "-t", "mangle", "-F", "FAKEHTTP_D", NULL},

        {"iptables", "-w", "-t", "mangle", "-D", "PREROUTING", "-j",
         "FAKEHTTP_S", NULL},

        {"iptables", "-w", "-t", "mangle", "-D", "POSTROUTING", "-j",
         "FAKEHTTP_D", NULL},

        {"iptables", "-w", "-t", "mangle", "-X", "FAKEHTTP_R", NULL},

        {"iptables", "-w", "-t", "mangle", "-X", "FAKEHTTP_S", NULL},

        {"iptables", "-w", "-t", "mangle", "-X", "FAKEHTTP_D", NULL}};

    cnt = sizeof(ipt_cmds) / sizeof(*ipt_cmds);
    for (i = 0; i < cnt; i++) {
        fh_execute_command(ipt_cmds[i], 1, NULL);
    }
}
