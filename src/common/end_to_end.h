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
/*
 * +-----+-----------------------------------------------+
 * | hdr | Data                                          |
 * +-----+-----------------------------------------------+
 *
 * +----//----------------------+----------------------------------------+
 * |             |  E2E Header  |   Data ---- N bytes ----               |
 * |             |<--64 bytes-->|<< Start of data buffer is Page Aligned |
 * |<---//----PAGE_SIZE bytes-->|                                        |
 * +----//----------------------+----------------------------------------+
 *
 */

#ifdef HAVE_SCHED_H
#include <sched.h>
#endif
#include "xni.h"

#define HOSTNAMELENGTH 1024

#define	E2E_ADDRESS_TABLE_ENTRIES 16
struct xdd_e2e_header {
	uint32_t 	e2eh_magic;  				// Magic Number - sanity check
	unsigned char e2eh_cookie[16];			// Magic Cookie - a safer check
	int32_t  	e2eh_worker_thread_number; 	// Sender's Worker Thread Number
	int64_t  	e2eh_sequence_number; 		// Sequence number of this operation
	nclk_t  	e2eh_send_time; 			// Time this packet was sent in global nano seconds 
	nclk_t  	e2eh_recv_time; 			// Time this packet was received in global nano seconds 
	int64_t  	e2eh_byte_offset; 			// Offset relative to the beginning of the file of where this data belongs
	int64_t  	e2eh_data_length; 			// Length of the user data in bytes for this operation 
};
typedef struct xdd_e2e_header xdd_e2e_header_t;

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

// Things used in the various end_to_end subroutines.
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#define FD_SETSIZE 256
#endif

#define MAXMIT_TCP     (1<<28)

/*
 * The xint_td_e2e structure contains variables that are referenced by the 
 * target thread.
 */
struct xint_td_e2e {
	char				*e2e_dest_hostname; 	// Name of the Destination machine 
	char				*e2e_src_hostname; 		// Name of the Source machine 
	char				*e2e_src_file_path;     // Full path of source file for destination restart file 
	time_t				e2e_src_file_mtime;     // stat -c %Y *e2e_src_file_path, i.e., last modification time
	in_addr_t			e2e_dest_addr;  		// Destination Address number of the E2E socket 
	in_port_t			e2e_dest_port;  		// Port number to use for the E2E socket 

	/* XNI data */
	xni_connection_t xni_conn;
};
typedef struct xint_td_e2e xint_td_e2e_t;
	
/*
 * The xint_e2e structure contains variables that are referenced by the 
 * target thread and worker thread.
 */
struct xint_e2e {
	char				*e2e_dest_hostname; 	// Name of the Destination machine 
	char				*e2e_src_hostname; 		// Name of the Source machine 
	char				*e2e_src_file_path;     // Full path of source file for destination restart file 
	time_t				e2e_src_file_mtime;     // stat -c %Y *e2e_src_file_path, i.e., last modification time
	in_addr_t			e2e_dest_addr;  		// Destination Address number of the E2E socket 
	in_port_t			e2e_dest_port;  		// Port number to use for the E2E socket 
	int32_t				e2e_sd;   				// Socket descriptor for the E2E message port 
	int32_t				e2e_nd;   				// Number of Socket descriptors in the read set 
	sd_t				e2e_csd[FD_SETSIZE];	// Client socket descriptors 
	fd_set				e2e_active;  			// This set contains the sockets currently active 
	fd_set				e2e_readset; 			// This set is passed to select() 
	struct sockaddr_in  e2e_sname; 				// used by setup_server_socket 
	uint32_t			e2e_snamelen; 			// the length of the socket name 
	struct sockaddr_in  e2e_rname; 				// used by destination machine to remember the name of the source machine 
	uint32_t			e2e_rnamelen; 			// the length of the source socket name 
	int32_t				e2e_current_csd; 		// the current csd used by the select call on the destination side
	int32_t				e2e_next_csd; 			// The next available csd to use 
	xdd_e2e_header_t	*e2e_hdrp;				// Pointer to the header portion of a packet
	unsigned char		*e2e_datap;				// Pointer to the data portion of a packet
	int32_t				e2e_header_size; 		// Size of the header portion of the buffer 
	int32_t				e2e_data_size; 			// Size of the data portion of the buffer
	int32_t				e2e_xfer_size; 			// Number of bytes per End to End request - size of data buffer plus size of E2E Header
	int32_t				e2e_send_status; 		// Current Send Status
	int32_t				e2e_recv_status; 		// Current Recv status
#define XDD_E2E_DATA_READY 	0xDADADADA 			// The magic number that should appear at the beginning of each message indicating data is present
#define XDD_E2E_EOF 	0xE0F0E0F0 				// The magic number that should appear in a message signaling and End of File
	int64_t				e2e_msg_sequence_number;// The Message Sequence Number of the most recent message sent or to be received
	int32_t				e2e_msg_sent; 			// The number of messages sent 
	int32_t				e2e_msg_recv; 			// The number of messages received 
	int64_t				e2e_prev_loc; 			// The previous location from a e2e message from the source 
	int64_t				e2e_prev_len; 			// The previous length from a e2e message from the source 
	int64_t				e2e_data_recvd; 		// The amount of data that is received each time we call xdd_e2e_dest_recv()
	int64_t				e2e_data_length; 		// The amount of data that is ready to be read for this operation 
	int64_t				e2e_total_bytes_written; // The total amount of data written across all restarts for this file
	nclk_t				e2e_wait_1st_msg;		// Time in nanosecs destination waited for 1st source data to arrive 
	nclk_t				e2e_first_packet_received_this_pass;// Time that the first packet was received by the destination from the source
	nclk_t				e2e_last_packet_received_this_pass;// Time that the last packet was received by the destination from the source
	nclk_t				e2e_first_packet_received_this_run;// Time that the first packet was received by the destination from the source
	nclk_t				e2e_last_packet_received_this_run;// Time that the last packet was received by the destination from the source
	nclk_t				e2e_sr_time; 			// Time spent sending or receiving data for End-to-End operation
	int32_t				e2e_address_table_host_count;	// Cumulative number of hosts represented in the e2e address table
	int32_t				e2e_address_table_port_count;	// Cumulative number of ports represented in the e2e address table
	int32_t				e2e_address_table_next_entry;	// Next available entry in the e2e_address_table
	xdd_e2e_ate_t		e2e_address_table[E2E_ADDRESS_TABLE_ENTRIES]; // Used by E2E to stripe over multiple IP Addresses

	/* XNI Target data */
	xni_connection_t xni_td_conn;

	/* XNI Worker data */
	xni_target_buffer_t xni_wd_buf;
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
