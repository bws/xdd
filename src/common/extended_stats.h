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
 *       Brad Settlemyer, DoE/ORNL (settlemyerbw@ornl.gov)
 *       Russell Cattelan, Digital Elves (russell@thebarn.com)
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */
struct xdd_extended_stats {
	// Longest and shortest op times - RESET AT THE START OF EACH PASS 
	// These values are only updated when the -extendedstats option is specified
	nclk_t		my_longest_op_time; 			// Longest op time that occured during this pass
	nclk_t		my_longest_read_op_time; 		// Longest read op time that occured during this pass
	nclk_t		my_longest_write_op_time; 		// Longest write op time that occured during this pass
	nclk_t		my_longest_noop_op_time; 		// Longest noop op time that occured during this pass
	nclk_t		my_shortest_op_time; 			// Shortest op time that occurred during this pass
	nclk_t		my_shortest_read_op_time; 		// Shortest read op time that occured during this pass
	nclk_t		my_shortest_write_op_time; 		// Shortest write op time that occured during this pass
	nclk_t		my_shortest_noop_op_time; 		// Shortest noop op time that occured during this pass

	int64_t		my_longest_op_bytes; 			// Bytes xfered when the longest op time occured during this pass
	int64_t	 	my_longest_read_op_bytes; 		// Bytes xfered when the longest read op time occured during this pass
	int64_t 	my_longest_write_op_bytes; 		// Bytes xfered when the longest write op time occured during this pass
	int64_t 	my_longest_noop_op_bytes; 		// Bytes xfered when the longest noop op time occured during this pass
	int64_t 	my_shortest_op_bytes; 			// Bytes xfered when the shortest op time occured during this pass
	int64_t 	my_shortest_read_op_bytes; 		// Bytes xfered when the shortest read op time occured during this pass
	int64_t 	my_shortest_write_op_bytes;		// Bytes xfered when the shortest write op time occured during this pass
	int64_t 	my_shortest_noop_op_bytes;		// Bytes xfered when the shortest noop op time occured during this pass

	int64_t		my_longest_op_number; 			// Operation Number when the longest op time occured during this pass
	int64_t	 	my_longest_read_op_number; 		// Operation Number when the longest read op time occured during this pass
	int64_t 	my_longest_write_op_number; 	// Operation Number when the longest write op time occured during this pass
	int64_t 	my_longest_noop_op_number; 		// Operation Number when the longest noop op time occured during this pass
	int64_t 	my_shortest_op_number; 			// Operation Number when the shortest op time occured during this pass
	int64_t 	my_shortest_read_op_number; 	// Operation Number when the shortest read op time occured during this pass
	int64_t 	my_shortest_write_op_number;	// Operation Number when the shortest write op time occured during this pass
	int64_t 	my_shortest_noop_op_number;		// Operation Number when the shortest noop op time occured during this pass

	int32_t		my_longest_op_pass_number;		// Pass Number when the longest op time occured during this pass
	int32_t		my_longest_read_op_pass_number;	// Pass Number when the longest read op time occured
	int32_t		my_longest_write_op_pass_number;// Pass Number when the longest write op time occured 
	int32_t		my_longest_noop_op_pass_number;	// Pass Number when the longest noop op time occured 
	int32_t		my_shortest_op_pass_number;		// Pass Number when the shortest op time occured 
	int32_t		my_shortest_read_op_pass_number;// Pass Number when the shortest read op time occured 
	int32_t		my_shortest_write_op_pass_number;// Pass Number when the shortest write op time occured 
	int32_t		my_shortest_noop_op_pass_number;// Pass Number when the shortest noop op time occured 
};
typedef struct xdd_extended_stats xdd_extended_stats_t;

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
