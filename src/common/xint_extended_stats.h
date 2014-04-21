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
struct xint_extended_stats {
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
typedef struct xint_extended_stats xint_extended_stats_t;

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
