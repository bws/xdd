/* Copyright (C) 1992-2010 I/O Performance, Inc. and the
 * United States Departments of Energy (DoE) and Defense (DoD)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named 'Copying'; if not, write to
 * the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139.
 */
/* Principal Author:
 *      Tom Ruwart (tmruwart@ioperformance.com)
 * Contributing Authors:
 *       Steve Hodson, DoE/ORNL, (hodsonsw@ornl.gov)
 *       Steve Poole, DoE/ORNL, (spoole@ornl.gov)
 *       Bradly Settlemyer, DoE/ORNL (settlemyerbw@ornl.gov)
 *       Russell Cattelan, Digital Elves (russell@thebarn.com)
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */

// The xdd_e2e_msg structure defines the layout ofthe trailer that is part of every e2e msg 
// and normally lives at the end of the msg packet
struct xdd_e2e_msg {
	uint32_t 	magic;  	/**< Magic number */
	int32_t  	sendqnum;  	/**< sender's myqnum  */
	int64_t  	sequence; 	/**< Sequence number */
	pclk_t  	sendtime; 	/**< Time this packet was sent in global pico seconds */
	pclk_t  	recvtime; 	/**< Time this packet was received in global pico seconds */
	int64_t  	location; 	/**< Starting location in bytes for this operation */
	int64_t  	length;  	/**< Length in bytes this operation */
};
typedef struct xdd_e2e_msg xdd_e2e_msg_t;

// The e2e structure contains the information that lives in the ptds and is used 
// during e2e operations
// The following "e2e_" members are for the End to End ( aka -e2e ) option
// The Target Option Flag will have either TO_E2E_DESTINATION or TO_E2E_SOURCE set to indicate
// which side of the E2E operation this target will act as.
//
#define PTDS_E2E_MAGIC 		0x07201959 			// The magic number that should appear at the beginning of each message 
#define PTDS_E2E_MAGIQ 		0x07201960 			// The magic number that should appear in a message signaling destination to quit 
struct e2e {
	char				*e2e_dest_hostname; 	// Name of the Destination machine 
	char				*e2e_src_hostname; 		// Name of the Source machine 
	struct hostent 		*e2e_dest_hostent; 		// For the destination host information 
	struct hostent 		*e2e_src_hostent; 		// For the source host information 
	in_addr_t			e2e_dest_addr;  		// Destination Address number of the E2E socket 
	in_port_t 			e2e_dest_base_port;  	// The first of one or more ports to be used by multiple queuethreads 
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
	int32_t				e2e_iosize;   			// number of bytes per End to End request 
	xdd_e2e_msg_t 		e2e_msg;  				// The message sent in from the destination 
	int64_t				e2e_msg_last_sequence;	// The last msg sequence number received 
	int32_t				e2e_msg_sent; 			// The number of messages sent 
	int32_t				e2e_msg_recv; 			// The number of messages received 
	int32_t				e2e_useUDP; 			// if <>0, use UDP instead of TCP from source to destination 
	int32_t				e2e_timedout; 			// if <>0, indicates recvfrom timedout...move on to next pass 
	int64_t				e2e_prev_loc; 			// The previous location from a e2e message from the source 
	int64_t				e2e_prev_len; 			// The previous length from a e2e message from the source 
	int64_t				e2e_data_ready; 		// The amount of data that is ready to be read in an e2e op 
	int64_t				e2e_data_length; 		// The amount of data that is ready to be read for this operation 
	pclk_t				e2e_wait_1st_msg;		// Time in picosecs destination waited for 1st source data to arrive 
	pclk_t				e2e_first_packet_received_this_pass;// Time that the first packet was received by the destination from the source
	pclk_t				e2e_last_packet_received_this_pass;// Time that the last packet was received by the destination from the source
	pclk_t				e2e_first_packet_received_this_run;// Time that the first packet was received by the destination from the source
	pclk_t				e2e_last_packet_received_this_run;// Time that the last packet was received by the destination from the source
	pclk_t				e2e_sr_time; 			// Time spent sending or receiving data for End-to-End operation
};
typedef struct e2e e2e_t;
