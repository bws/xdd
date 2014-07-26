/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-2013 I/O Performance, Inc.
 * Copyright (C) 2009-2013 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

#ifdef HAVE_SCHED_H
#include <sched.h>
#endif
#include "xni.h"

#define HOSTNAMELENGTH 1024

#define	E2E_ADDRESS_TABLE_ENTRIES 16

struct xdd_e2e_address_table_entry {
    char 	*address;					// Pointer to the ASCII string of the address 
    char 	hostname[HOSTNAMELENGTH];	// the ASCII string of the hostname associated with address 
    int	 	base_port;					// The Base Port number associated with this address entry
    int	 	port_count;					// Number of ports from "port" to "port+nports-1"
#if (HAVE_CPU_SET_T)
    cpu_set_t cpu_set;
#endif
};
typedef struct xdd_e2e_address_table_entry xdd_e2e_ate_t;
struct xdd_e2e_address_table {
	int		number_of_entries;		// Number of address table entries
	struct	xdd_e2e_address_table_entry e2eat_entry[1];
};
typedef struct xdd_e2e_address_table xdd_e2e_at_t;

/*
 * The xint_e2e structure contains variables that are referenced by the 
 * target thread and worker thread.
 */
struct xint_e2e {
	in_addr_t			e2e_dest_addr;  		// Destination Address number of the E2E socket 
	int32_t				e2e_send_status; 		// Current Send Status
	int64_t				e2e_msg_sequence_number;// The Message Sequence Number of the most recent message sent or to be received
	nclk_t				e2e_wait_1st_msg;		// Time in nanosecs destination waited for 1st source data to arrive 
	nclk_t				e2e_sr_time; 			// Time spent sending or receiving data for End-to-End operation
	int32_t				e2e_address_table_host_count;	// Cumulative number of hosts represented in the e2e address table
	int32_t				e2e_address_table_port_count;	// Cumulative number of ports represented in the e2e address table
	xdd_e2e_ate_t		e2e_address_table[E2E_ADDRESS_TABLE_ENTRIES]; // Used by E2E to stripe over multiple IP Addresses

	/* XNI Target data */
	xni_connection_t *xni_td_connections;  // One connection per host
	int xni_td_connections_count;  // Number of connection objects
	pthread_mutex_t *xni_td_connection_mutexes;  // To synchronize connection establishment; one per connection

	/* XNI Worker data */
	xni_target_buffer_t xni_wd_buf;
	int received_eof;  // TRUE when the source has signaled EOF to this destination worker
	int address_table_index;  // Index into e2e_address_table for the address assigned to this worker
}; // End of struct xint_e2e definition
typedef struct xint_e2e xint_e2e_t;

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  default-tab-width: 4
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
