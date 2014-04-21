/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-23 I/O Performance, Inc.
 * Copyright (C) 2009-23 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */
/*
 * This file contains the subroutines necessary to perform data verification
 */
#include "xint.h"

/*----------------------------------------------------------------------------*/
/* xdd_verify_checksum() - Verify data checksum of the data buffer  
 * Returns the number of miscompare errors.
 */
int32_t
xdd_verify_checksum(worker_data_t *wdp, int64_t current_op) {
	fprintf(xgp->errout, "%s: xdd_verify_checksum: ERROR: NOT IMPLEMENTED YET\n", xgp->progname);
	return(0);
} // end of xdd_verify_checksum()

/*----------------------------------------------------------------------------*/
/* xdd_Verify_hex() - Verify hex data pattern in the data buffer  
 * Returns the number of miscompare errors.
 * This routine assumes that the specified hex data pattern and replication 
 * factor has been previously written to the media that was just read and 
 * is being verified. 
 * It is further assumed that the data pattern and data pattern lenggth
 * are in tdp->td_dpp->data_pattern and tdp->td_dpp->data_pattern_length respectively. This is
 * done by the datapattern function in the parse.c file. If the data_pattern_option
 * of "DP_REPLICATE_PATTERN" was specified as well, then the data comparison is
 * made throughout the data buffer. Otherwise only the first N bytes are compared
 * against the data pattern where N is equal to tdp->td_dpp->data_pattern_length. Cool, huh?
 */
int32_t
xdd_verify_hex(worker_data_t *wdp, int64_t current_op) {
	target_data_t	*tdp;
	size_t i;
	int32_t errors;
	int32_t remaining;
	int32_t offset;
	unsigned char *patternp, *bufferp;


	tdp = wdp->wd_tdp;

	if (tdp->td_dpp->data_pattern_options & DP_REPLICATE_PATTERN) 
		remaining = wdp->wd_task.task_xfer_size;
	else remaining = tdp->td_dpp->data_pattern_length;

	offset = 0;
	bufferp = wdp->wd_task.task_datap;
	errors = 0;
	while (remaining) {
		patternp = tdp->td_dpp->data_pattern;
		for (i=0; i<tdp->td_dpp->data_pattern_length; i++, patternp++, bufferp++) {
			if (*patternp != *bufferp) {
				fprintf(xgp->errout,"%s: xdd_verify_hex: Target %d Worker Thread %d: ERROR: Content mismatch on op %lld at %d bytes into block %lld, expected 0x%02x, got 0x%02x\n",
					xgp->progname, 
					tdp->td_target_number, 
					wdp->wd_worker_number, 
					(long long int)current_op,
					offset, 
					(long long int)(wdp->wd_task.task_byte_offset/tdp->td_block_size), 
					*patternp, 
					*bufferp);

				errors++;
			}
			offset++;
			remaining--;
			if (remaining <= 0) break;
		}
	}
	return(errors);
} // end of xdd_verify_hex()
/*----------------------------------------------------------------------------*/
/* xdd_verify_sequence() - Verify data contents  of a sequenced data pattern 
 * Returns the number of miscompare errors.
 * The 8-byte sequence number data patter is specified as "-datapattern sequenced". This will cause xdd to write a
 * sequence of 8-byte integers that start at 0 and increment by 8 until the end of the buffer is reached. 
 * The hex representation of the sequenced data pattern would look like so:
 *      0000000000000000 0000000000000008 0000000000000010 0000000000000018...
 * If there is a "prefix" in the sequence pattern then the specified prefix is included in the comparison.
 * For example, if the prefix is 0x0123 then the hex representation of the sequenced data pattern would look like so:
 *      0123000000000000 0123000000000008 0123000000000010 0123000000000018...
 * Keep in mind that this example is shown in BIG endian so as not to confuse myself.
 */
int32_t
xdd_verify_sequence(worker_data_t *wdp, int64_t current_op) {
	target_data_t	*tdp;
	size_t  		i,j;
	uint64_t	  		errors;
	uint64_t 		expected_data;
	uint64_t 		*uint64p;
	unsigned char 	*ucp;        /* A temporary unsigned char pointer */
 

	tdp = wdp->wd_tdp;

	uint64p = (uint64_t *)wdp->wd_task.task_datap;
	errors = 0;
	for (i = 0; i < wdp->wd_task.task_xfer_size; i+=(sizeof(wdp->wd_task.task_byte_offset))) {
		expected_data = wdp->wd_task.task_byte_offset + i;
		if (tdp->td_dpp->data_pattern_options & DP_PATTERN_PREFIX) { // OR-in the pattern prefix
			expected_data |= tdp->td_dpp->data_pattern_prefix_binary;
		} 
		if (tdp->td_dpp->data_pattern_options & DP_INVERSE_PATTERN)
			expected_data ^= 0xffffffffffffffffLL; // 1's compliment of the expected data 

		if (*uint64p != expected_data) { // If the expected_data pattern is not what we think it should be then scream!
			//Check how many errors we've had, if too many, then don't print data
			if (errors <= xgp->max_errors_to_print) {
				fprintf(xgp->errout,"%s: xdd_verify_sequence: Target %d Worker Thread %d: ERROR: Sequence mismatch on op number %lld at %zd bytes into block %lld\n",
					xgp->progname, 
					tdp->td_target_number, 
					wdp->wd_worker_number, 
					(long long int)current_op,
					i, 
					(long long int)(wdp->wd_task.task_byte_offset/tdp->td_block_size));

				fprintf(xgp->errout, "expected 0x");
				for (j=0, ucp=(unsigned char *)&expected_data; j<sizeof(uint64_t); j++, ucp++) {
					fprintf(xgp->errout, "%02x",*ucp);
				}
				fprintf(xgp->errout, ", got 0x");
				for (j=0, ucp=(unsigned char *)uint64p; j<sizeof(uint64_t); j++, ucp++) {
					fprintf(xgp->errout, "%02x",*ucp);
				}
				fprintf(xgp->errout, "\n");
			}
			errors++;
		}
		uint64p++;
	} // end of FOR loop that looks at all locations 
	//print out remaining error count if exceeded max
    if (errors > xgp->max_errors_to_print) {
		fprintf(xgp->errout,"%s: xdd_verify_sequence: Target %d Worker Thread %d: ERROR: ADDITIONAL Data Buffer Content mismatches = %lld\n",
			    xgp->progname, 
				tdp->td_target_number, 
				wdp->wd_worker_number, 
				(long long int)(errors - (xgp->max_errors_to_print)));
	}
	return(errors);
} // end of xdd_verify_sequence() 

/*----------------------------------------------------------------------------*/
/* xdd_verify_singlechar() - Verify data contents of a single character data pattern 
 * Returns the number of miscompare errors. 
 * The single-byte data pattern is specified simply by giving the -datapattern a single character to write to the device. 
 * If that same character is specified for a read operation with the -verify option then that character will be compared with the
 * contents of the I/O buffer for every block read.
 */
int32_t
xdd_verify_singlechar(worker_data_t *wdp, int64_t current_op) {
	target_data_t	*tdp;
	size_t  i;
	int32_t  errors;
	unsigned char *ucp;
 

	tdp = wdp->wd_tdp;

	ucp = wdp->wd_task.task_datap;
	errors = 0;
	for (i = 0; i < wdp->wd_task.task_xfer_size; i++) {
		if (*ucp != *(tdp->td_dpp->data_pattern)) {
			fprintf(xgp->errout,"%s: xdd_verify_singlechar: Target %d Worker Thread %d: ERROR: Content mismatch on op number %lld at %zd bytes into block %lld, expected 0x%02x, got 0x%02x\n",
				xgp->progname, 
				tdp->td_target_number, 
				wdp->wd_worker_number, 
				(long long int)current_op,
				i, 
				(unsigned long long)(wdp->wd_task.task_byte_offset/tdp->td_block_size), 
				*(tdp->td_dpp->data_pattern), 
				*ucp);
		errors++;
		} /* End printing error message */
		ucp++;
	} // end of FOR statement that looks at all bytes

	return(errors);

} // end of xdd_verify_singlechar() 

/*----------------------------------------------------------------------------*/
/* xdd_verify_contents() - Verify data contents  
 * Returns the number of miscompare errors.
 * There are various kinds of data patterns that xdd can read back for comparison. 
 * The user is responsible for using xdd to write the desired data pattern to the device 
 * and then request the proper verification / data pattern.
 * The  data patterns currently supported are: single byte data, hex digits, ascii strings, and 8-byte sequence numbers. 
 * There is a separate subroutine in this file that handles the verification for each type of data pattern.
 * The subroutine names are obvious. If not, you should not be reading this.
 */
int32_t
xdd_verify_contents(worker_data_t *wdp, int64_t current_op) {
	target_data_t	*tdp;
	int32_t  errors;

	tdp = wdp->wd_tdp;

	errors = 0;
	/* Verify the contents of the buffer is equal to the specified data pattern */
	if (tdp->td_dpp->data_pattern_options & DP_SEQUENCED_PATTERN) { // Lets look at a sequenced data pattern
		errors = xdd_verify_sequence(wdp, current_op);
		return(errors);
	}

	if (tdp->td_dpp->data_pattern_options & DP_HEX_PATTERN) { // Lets look at a HEX data pattern
		errors = xdd_verify_hex(wdp, current_op);
		return(errors);
	}

	if (tdp->td_dpp->data_pattern_options & DP_SINGLECHAR_PATTERN) { // Lets look at a single character data pattern
		errors = xdd_verify_singlechar(wdp, current_op);
		return(errors);
	}

	// If we get here then the data pattern was either not specified or the data pattern type was not recognized.
	fprintf(xgp->errout, "%s: xdd_verify_contents: Target %d Worker Thread %d: ERROR: Data verification request not understood. No verification possible.\n",
				xgp->progname, 
				tdp->td_target_number, 
				wdp->wd_worker_number);
	return(0);
	
} // end of xdd_verify_contents()  

/*----------------------------------------------------------------------------*/
/* xdd_verify_location() - Verify data location 
 * This routine gets the current bytes location that is located in the first
 * 8-bytes of the rw buffer and compares it to the current byte location that
 * the calling routine specified in the worker_data_t->my_current_byte_location. If the
 * two do not match then we are not in Kansas anymore. Print an error message
 * and return a 1. Otherwise, everything is peachy, simply return a 0.
 * Returns the number of miscompare errors - 0 or 1 in this case.
 */
int32_t
xdd_verify_location(worker_data_t *wdp, int64_t current_op) {
	target_data_t	*tdp;
	int32_t  errors;
	uint64_t current_position;

	tdp = wdp->wd_tdp;

	errors = 0;
	current_position = *(uint64_t *)wdp->wd_task.task_datap;
	if (current_position != tdp->td_counters.tc_current_byte_offset) {
		errors++;
		fprintf(xgp->errout,"%s: xdd_verify_location: Target %d Worker Thread %d: ERROR: op number %lld: Data Buffer Sequence mismatch - expected %lld, got %lld\n",
			xgp->progname, 
			tdp->td_target_number, 
			wdp->wd_worker_number, 
			(long long int)tdp->td_counters.tc_current_op_number, 
			(long long int)tdp->td_counters.tc_current_byte_offset, 
			(long long int)current_position);

		fflush(xgp->errout);
	}
	return(errors);
} // end of xdd_verify_location() 

/*----------------------------------------------------------------------------*/
/* xdd_verify() - Verify data location and/or contents  
 * Returns the number of miscompare errors.
 */
int32_t
xdd_verify(worker_data_t *wdp, int64_t current_op) {
	target_data_t	*tdp;
	int32_t  errors;


	tdp = wdp->wd_tdp;

   /* Since the last operation was a read operation check to see if a sequenced data pattern
	* was specified. If so, then we need to verify that what we read has the correct 
	* sequence number(s) in it.
	*/
	if (!(tdp->td_target_options & (TO_VERIFY_CONTENTS | TO_VERIFY_LOCATION))) { // If we don't need to verify location or contents of the buffer, then just return.
		fprintf(xgp->errout,"%s: xdd_verify: Target %d Worker Thread %d: ERROR: Data verification type <location or contents> not specified - No verification performed.\n",
			xgp->progname,
			tdp->td_target_number,
			wdp->wd_worker_number);
		return(0);
	}

	// Looks like we need to verify something...
	if (tdp->td_target_options & TO_VERIFY_LOCATION) /* Assumes that the data pattern was sequenced. If not, there will be LOTS o' errors. */
		 errors = xdd_verify_location(wdp, current_op);
	else errors = xdd_verify_contents(wdp, current_op);

	return(errors);
} /* End of xdd_verify() */
 
 
