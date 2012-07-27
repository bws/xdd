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

#if (LINUX)
#define _GNU_SOURCE
#include <sched.c>
#endif

#define	E2E_ADDRESS_TABLE_ENTRIES 16
struct xdd_e2e_header {
	uint32_t 	magic;  			/**< Magic number */
	int32_t  	sendqnum;  			/**< Sender's QThread Number  */
	int64_t  	sequence; 			/**< Sequence number */
	nclk_t  	sendtime; 			/**< Time this packet was sent in global nano seconds */
	nclk_t  	recvtime; 			/**< Time this packet was received in global nano seconds */
	int64_t  	location; 			/**< Starting location in bytes for this operation relative to the beginning of the file*/
	int64_t  	length;  			/**< Length of the user data in bytes this operation */
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
#define FD_SETSIZE 128
#endif

#define MAXMIT_TCP     (1<<28)


