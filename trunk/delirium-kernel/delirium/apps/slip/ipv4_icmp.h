#ifndef __IPV4_ICMP_H
#define __IPV4_ICMP_H

#include "ipv4.h"

/***************************************************
 *
 * ICMP for IPv4
 *
 *
 * Still to implement:
 * 	Timestamp, Timestamp Reply, Information Request, Information Reply
 *
 */

#define ICMP_PRELUDE	\
	u_int8_t	type;\
	u_int8_t	code;\
	u_int16_t	checksum;

struct IPv4_ICMP_Header {
	ICMP_PRELUDE
} __packme;


/* ICMP Type 8: Echo 
 * ICMP Type 0: Echo Reply */

#define IPV4_ICMP_ECHO			8
#define IPV4_ICMP_ECHO_REPLY		0

/* Code: 0 if identifier and sequence fields are valid */
#define IPV4_ICMP_ECHO_VALID_ID_SEQ	0

struct IPv4_ICMP_Echo {
	ICMP_PRELUDE
	u_int16_t	identifier;
	u_int16_t	sequence_number;
} __packme;

/* ICMP Type 3: Destination unreachable */
#define IPV4_ICMP_DEST_UNREACH		3

/* Code field: Reasons for being unreachable */
#define IPV4_ICMP_DU_NET		0	// Network Unreachable
#define IPV4_ICMP_DU_HOST		1	// Host Unreachable
#define IPV4_ICMP_DU_PROTOCOL		2	// Protocol Unreachable
#define IPV4_ICMP_DU_PORT		3	// Port Unreachable
#define IPV4_ICMP_DU_NEED_TO_FRAGMENT	4	// DF set and MTU too high
#define IPV4_ICMP_DU_SOURCE_ROUTE_FAIL	5	// Source route failed

struct IPv4_ICMP_Unreachable {
	ICMP_PRELUDE
	u_int32_t		_unused;
	struct IPv4_Header	orig_header;

	/* 
	 * Context contains any IPv4 option fields, and then
	 * the first 64 bits (8 octets) of the payload of the
	 * message
	 */
	u_int8_t		context[];
} __packme;


/* ICMP Type 4: Source quench */

/* Code field: Always 0 */

struct IPv4_ICMP_Source_Quench {
	ICMP_PRELUDE
	u_int32_t		_unused;
	struct IPv4_Header	orig_header;
	u_int8_t		context[];
} __packme;

/* ICMP Type 5: Redirect */

/* Code field: what to redirect */
#define IPV4_ICMP_REDIR_NET		0
#define IPV4_ICMP_REDIR_HOST		1
#define IPV4_ICMP_REDIR_TOS_NET		2
#define IPV4_ICMP_REDIR_TOS_HOST	3

struct IPv4_ICMP_Redirect {
	ICMP_PRELUDE
	IPv4_Address		gateway; // Where to redir to
	struct IPv4_Header	orig_header; 
	u_int8_t		context[];
} __packme;

/* ICMP Type 11: Time exceeded */
#define IPV4_ICMP_TIME_EXCEEDED		11

/* Code field: Reason */
#define IPV4_ICMP_TE_TTL	0	// Time to live reached 0 in transit
#define IPV4_ICMP_TE_LOSTFRAG	1	// Couldn't find all fragments in time

struct IPv4_ICMP_Time_Exceeded {
	ICMP_PRELUDE
	u_int32_t		_unused;
	struct IPv4_Header	orig_header;
	u_int8_t		context[];
} __packme;


/* ICMP Type 12: Parameter problem */
#define IPV4_ICMP_PARAMATER_PROBLEM	12

/* Code field: 0: Offset in original packet header to error */
#define IPV4_ICMP_PP_PTR	0

struct IPv4_ICMP_Param_Problem {
	ICMP_PRELUDE
	u_int8_t		error_ptr;
	u_int8_t		_unused_a;
	u_int16_t		_unused_b;
	struct IPv4_Header	orig_header;
	u_int8_t		context[];
} __packme;

#endif
