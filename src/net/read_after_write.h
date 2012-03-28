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

/** Messaging structure for read-after-write processes */
struct xdd_raw_msg {
	uint32_t			magic;					// Magic number 
	uint32_t			filler; 
	int64_t				sequence;				// Sequence number 
	nclk_t				sendtime;				// Time this packet was sent in global nano seconds 
	nclk_t				recvtime;				// Time this packet was received in global nano seconds 
	int64_t				location;				// Starting location in bytes for this operation 
	int64_t				length;					// Length in bytes this operation 
}; 
typedef struct xdd_raw_msg xdd_raw_msg_t;

struct	xdd_raw	{
	char				*raw_myhostname; 		// Hostname of the reader machine as seen by the reader 
	char				*raw_hostname; 			// Name of the host doing the reading in a read-after-write 
	struct hostent 		*raw_hostent; 			// for the reader/writer host information 
	in_port_t			raw_port;  				// Port number to use for the read-after-write socket 
	in_addr_t			raw_addr;  				// Address number of the read-after-write socket
	int32_t				raw_lag;  				// Number of blocks the reader should lag behind the writer 
#define PTDS_RAW_STAT 0x00000001  				// Read-after-write should use stat() to trigger read operations 
#define PTDS_RAW_MP   0x00000002  				// Read-after-write should use message passing from the writer to trigger read operations 
	uint32_t			raw_trigger; 			// Read-After-Write trigger mechanism 
	int32_t				raw_sd;   				// Socket descriptor for the read-after-write message port 
	int32_t				raw_nd;   				// Number of Socket descriptors in the read set 
	sd_t				raw_csd[FD_SETSIZE];	// Client socket descriptors 
	fd_set				raw_active;  			// This set contains the sockets currently active 
	fd_set				raw_readset; 			// This set is passed to select() 
	struct sockaddr_in  raw_sname; 				// used by setup_server_socket 
	uint32_t			raw_snamelen; 			// the length of the socket name 
	int32_t				raw_current_csd; 		// the current csd used by the raw reader select call 
	int32_t				raw_next_csd; 			// The next available csd to use 
#define PTDS_RAW_MAGIC 0x07201958 				// The magic number that should appear at the beginning of each message 
	xdd_raw_msg_t		raw_msg;  				// The message sent in from the writer 
	int64_t				raw_msg_last_sequence;	// The last raw msg sequence number received 
	int32_t				raw_msg_sent; 			// The number of messages sent 
	int32_t				raw_msg_recv; 			// The number of messages received 
	int64_t				raw_prev_loc; 			// The previous location from a RAW message from the source 
	int64_t				raw_prev_len; 			// The previous length from a RAW message from the source 
	int64_t				raw_data_ready; 		// The amount of data that is ready to be read in an RAW op 
	int64_t				raw_data_length; 		// The amount of data that is ready to be read for this operation 
}; 
typedef struct xdd_raw xdd_raw_t;
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */
