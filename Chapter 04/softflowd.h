/*
 * Copyright (c) 2002 Damien Miller.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SOFTFLOWD_H
#define _SOFTFLOWD_H

#include "common.h"
#include "sys-tree.h"
#include "freelist.h"
#include "treetype.h"

/* User to setuid to and directory to chroot to when we drop privs */
#ifndef PRIVDROP_USER
# define PRIVDROP_USER		"nobody"
#endif

#ifndef PRIVDROP_CHROOT_DIR
# define PRIVDROP_CHROOT_DIR	"/var/empty"
#endif
/*
 * Capture length for libpcap: Must fit the link layer header, plus 
 * a maximally sized ip/ipv6 header and most of a TCP header
 */
#define LIBPCAP_SNAPLEN_V4		96
#define LIBPCAP_SNAPLEN_V6		160

/*
 * Timeouts
 */
#define DEFAULT_TCP_TIMEOUT		3600
#define DEFAULT_TCP_RST_TIMEOUT		120
#define DEFAULT_TCP_FIN_TIMEOUT		300
#define DEFAULT_UDP_TIMEOUT		300
#define DEFAULT_ICMP_TIMEOUT		300
#define DEFAULT_GENERAL_TIMEOUT		3600
#define DEFAULT_MAXIMUM_LIFETIME	(3600*24*7)
#define DEFAULT_EXPIRY_INTERVAL		60

/*
 * Default maximum number of flow to track simultaneously 
 * 8192 corresponds to just under 1Mb of flow data
 */
#define DEFAULT_MAX_FLOWS	8192

/* Store a couple of statistics, maybe more in the future */
struct STATISTIC {
	double min, mean, max;
};

/* Flow tracking levels */
#define TRACK_FULL		1	/* src/dst/addr/port/proto/tos 6-tuple */
#define TRACK_IP_PROTO_PORT	2	/* src/dst/addr/port/proto 5-tuple */
#define TRACK_IP_PROTO		3	/* src/dst/proto 3-tuple */
#define TRACK_IP_ONLY		4	/* src/dst tuple */
#define TRACK_FULL_VLAN		5	/* src/dst/addr/port/proto/tos/vlanid 7-tuple */

/*
 * This structure contains optional information carried by Option Data
 * Record.
 */
struct OPTION {
	uint32_t sample;
	pid_t meteringProcessId;
};

struct FLOWTRACKPARAMETERS {
	unsigned int num_flows;			/* # of active flows */
	unsigned int max_flows;			/* Max # of active flows */
	u_int64_t next_flow_seq;		/* Next flow ID */

	/* Stuff related to flow export */
	struct timeval system_boot_time;	/* SysUptime */
	int track_level;			/* See TRACK_* above */

	/* Flow timeouts */
	int tcp_timeout;			/* Open TCP connections */
	int tcp_rst_timeout;			/* TCP flows after RST */
	int tcp_fin_timeout;			/* TCP flows after bidi FIN */
	int udp_timeout;			/* UDP flows */
	int icmp_timeout;			/* ICMP flows */
	int general_timeout;			/* Everything else */
	int maximum_lifetime;			/* Maximum life for flows */
	int expiry_interval;			/* Interval between expiries */ 

	/* Statistics */
	u_int64_t total_packets;		/* # of good packets */
	u_int64_t non_sampled_packets;		/* # of not sampled packets */
	u_int64_t frag_packets;			/* # of fragmented packets */
	u_int64_t non_ip_packets;		/* # of not-IP packets */
	u_int64_t bad_packets;			/* # of bad packets */
	u_int64_t flows_expired;		/* # expired */
	u_int64_t flows_exported;		/* # of flows sent */
	u_int64_t flows_dropped;		/* # of flows dropped */
	u_int64_t flows_force_expired;		/* # of flows forced out */
	u_int64_t packets_sent;			/* # netflow packets sent */
	u_int64_t records_sent;			/* # netflow records sent */
	struct STATISTIC duration;		/* Flow duration */
	struct STATISTIC octets;		/* Bytes (bidir) */
	struct STATISTIC packets;		/* Packets (bidir) */

	/* Per protocol statistics */
	u_int64_t flows_pp[256];
	u_int64_t octets_pp[256];
	u_int64_t packets_pp[256];
	struct STATISTIC duration_pp[256];

	/* Timeout statistics */
	u_int64_t expired_general;
	u_int64_t expired_tcp;
	u_int64_t expired_tcp_rst;
	u_int64_t expired_tcp_fin;
	u_int64_t expired_udp;
	u_int64_t expired_icmp;
	u_int64_t expired_maxlife;
	u_int64_t expired_overbytes;
	u_int64_t expired_maxflows;
	u_int64_t expired_flush;

	/* Optional information */
	struct OPTION option;
	char time_format;
	u_int8_t bidirection;
};
/*
 * This structure is the root of the flow tracking system.
 * It holds the root of the tree of active flows and the head of the
 * tree of expiry events. It also collects miscellaneous statistics
 */
struct FLOWTRACK {
	/* The flows and their expiry events */
	FLOW_HEAD(FLOWS, FLOW) flows;		/* Top of flow tree */
	EXPIRY_HEAD(EXPIRIES, EXPIRY) expiries;	/* Top of expiries tree */

	struct freelist flow_freelist;		/* Freelist for flows */
	struct freelist expiry_freelist;	/* Freelist for expiry events */

	struct FLOWTRACKPARAMETERS param;
};

/*
 * This structure is an entry in the tree of flows that we are 
 * currently tracking. 
 *
 * Because flows are matched _bi-directionally_, they must be stored in
 * a canonical format: the numerically lowest address and port number must
 * be stored in the first address and port array slot respectively.
 */
struct FLOW {
	/* Housekeeping */
	struct EXPIRY *expiry;			/* Pointer to expiry record */
	FLOW_ENTRY(FLOW) trp;			/* Tree pointer */

	/* Per-flow statistics (all in _host_ byte order) */
	u_int64_t flow_seq;			/* Flow ID */
	struct timeval flow_start;		/* Time of creation */
	struct timeval flow_last;		/* Time of last traffic */

	/* Per-endpoint statistics (all in _host_ byte order) */
	u_int32_t octets[2];			/* Octets so far */
	u_int32_t packets[2];			/* Packets so far */

	/* Flow identity (all are in network byte order) */
	int af;					/* Address family of flow */
	u_int32_t ip6_flowlabel[2];		/* IPv6 Flowlabel */
	union {
		struct in_addr v4;
		struct in6_addr v6;
	} addr[2];				/* Endpoint addresses */
	u_int16_t port[2];			/* Endpoint ports */
	u_int8_t tcp_flags[2];			/* Cumulative OR of flags */
	u_int8_t tos[2];			/* Tos */
        u_int16_t vlanid;                       /* vlanid */
	u_int8_t protocol;			/* Protocol */
	
	char dnsquery[128];
	char httpurl[128];
	char radius_username[32];
	char ldap_username[32];
};

/*
 * This is an entry in the tree of expiry events. The tree is used to 
 * avoid traversion the whole tree of active flows looking for ones to
 * expire. "expires_at" is the time at which the flow should be discarded,
 * or zero if it is scheduled for immediate disposal. 
 *
 * When a flow which hasn't been scheduled for immediate expiry registers 
 * traffic, it is deleted from its current position in the tree and 
 * re-inserted (subject to its updated timeout).
 *
 * Expiry scans operate by starting at the head of the tree and expiring
 * each entry with expires_at < now
 * 
 */
struct EXPIRY {
	EXPIRY_ENTRY(EXPIRY) trp;		/* Tree pointer */
	struct FLOW *flow;			/* pointer to flow */

	u_int32_t expires_at;			/* time_t */
	enum { 
		R_GENERAL, R_TCP, R_TCP_RST, R_TCP_FIN, R_UDP, R_ICMP, 
		R_MAXLIFE, R_OVERBYTES, R_OVERFLOWS, R_FLUSH
	} reason;
};

/* Prototype for functions shared from softflowd.c */
u_int32_t timeval_sub_ms(const struct timeval *t1, const struct timeval *t2);

/* Prototypes for functions to send NetFlow packets, from netflow*.c */
int send_netflow_v1(struct FLOW **flows, int num_flows, int nfsock,
		    u_int16_t ifidx, struct FLOWTRACKPARAMETERS *param,
		    int verbose_flag);
int send_netflow_v5(struct FLOW **flows, int num_flows, int nfsock,
		    u_int16_t ifidx, struct FLOWTRACKPARAMETERS *param,
		    int verbose_flag);
int send_netflow_v9(struct FLOW **flows, int num_flows, int nfsock,
		    u_int16_t ifidx, struct FLOWTRACKPARAMETERS *param,
		    int verbose_flag);
int send_ipfix(struct FLOW **flows, int num_flows, int nfsock,
	       u_int16_t ifidx, struct FLOWTRACKPARAMETERS *param,
	       int verbose_flag);
int send_ipfix_bidirection(struct FLOW **flows, int num_flows, int nfsock,
			   u_int16_t ifidx, 
			   struct FLOWTRACKPARAMETERS *param,
			   int verbose_flag);

/* Force a resend of the flow template */
void netflow9_resend_template(void);
void ipfix_resend_template(void);

#define PEN_FL0WER		54084

#endif /* _SOFTFLOWD_H */
