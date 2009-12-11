/* Copyright (C) 1992-2009 I/O Performance, Inc.
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

/**
 * @author Tom Ruwart (tmruwart@ioperformance.com)
 * @author I/O Performance, Inc.
 *
 * @file end_to_end.h
 * Defines the messaging structure for write-after-read processes
 */

/** Results structure for write-after-read processes */
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

/** Timing Profile structure swh notes this is NOT used anywhere !!!!! */
struct xdd_timing_profile {
pclk_t        base_time;  				/**< This is time t0 and is the same for all targets and all queue threads */
pclk_t        open_start_time; 			/**< Time just before the open is issued for this target */
pclk_t        open_end_time; 			/**< Time just after the open completes for this target */
pclk_t        my_start_time; 			/**< This is the time stamp just before the first actual I/O operation for this T/Q thread */
pclk_t        my_current_start_time; 	/**< This is the time stamp of the current I/O Operation for thsi T/Q Thread */
pclk_t        my_end_time; 				/**< This is the time stamp just after the last I/O operation performed by this T/Q thread */
pclk_t        my_current_end_time; 		/**< This is the time stamp of the current I/O Operation for thsi T/Q Thread */
pclk_t        my_current_elapsed_time; 	/**< This is the time stamp of the current I/O Operation for thsi T/Q Thread */
};
typedef struct xdd_timing_profile xdd_timing_profile_t;
 
/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vim: ts=8 sts=8 sw=8 noexpandtab
 */
