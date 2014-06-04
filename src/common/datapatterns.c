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
 * This file contains the subroutines that initialize various data patterns
 * used by XDD
 */
#define XDD_DATA_PATTERN
#include <assert.h>
#include "xint.h"


int xdd_data_pattern_init(xint_data_pattern_t* xdp)
{
    assert(0 != xdp);
    memset(xdp, 0, sizeof(*xdp));
    return 0;
}

/*----------------------------------------------------------------------------*/
/* xdd_pattern_buffer() - init the I/O buffer with the appropriate pattern
 * This routine will put the requested pattern in the rw buffer.
 */
void
xdd_datapattern_buffer_init(worker_data_t *wdp) {
	target_data_t	*tdp;
    int32_t i;
    int32_t pattern_length; // Length of the pattern
    size_t remaining_length; // Length of the space in the pattern buffer
    unsigned char    *ucp;          // Pointer to an unsigned char type, duhhhh
    uint32_t *lp;			// pointer to a pattern
    xint_data_pattern_t	*dpp;


	tdp = wdp->wd_tdp;
    dpp = tdp->td_dpp;
    if (dpp->data_pattern_options & DP_RANDOM_PATTERN) { // A nice random pattern
		lp = (uint32_t *)wdp->wd_task.task_datap;
		xgp->random_initialized = 0;
		xgp->random_init_seed = 72058; // Backward compatibility with older xdd versions
		/* Set each four-byte field in the I/O buffer to a random integer */
		for(i = 0; i < (int32_t)(tdp->td_xfer_size / sizeof(int32_t)); i++ ) {
	    	*lp=xdd_random_int();
	    	lp++;
		}
    } else if (dpp->data_pattern_options & DP_RANDOM_BY_TARGET_PATTERN) { // A nice random pattern, unique by target number
		lp = (uint32_t *)wdp->wd_task.task_datap;
		xgp->random_initialized = 0;
		xgp->random_init_seed = (tdp->td_target_number+1); 
		/* Set each four-byte field in the I/O buffer to a random integer */
		for(i = 0; i < (int32_t)(tdp->td_xfer_size / sizeof(int32_t)); i++ ) {
	    	*lp=xdd_random_int();
	    	lp++;
		}
    } else if ((dpp->data_pattern_options & DP_ASCII_PATTERN) ||
	     (dpp->data_pattern_options & DP_HEX_PATTERN)) { // put the pattern that is in the pattern buffer into the io buffer
		// Clear out the buffer before putting in the string so there are no strange characters in it.
		memset(wdp->wd_task.task_datap,'\0',tdp->td_xfer_size);
		if (dpp->data_pattern_options & DP_REPLICATE_PATTERN) { // Replicate the pattern throughout the buffer
	    	ucp = (unsigned char *)wdp->wd_task.task_datap;
	    	remaining_length = tdp->td_xfer_size;
	    	while (remaining_length) { 
				if (dpp->data_pattern_length < remaining_length) 
		    		pattern_length = dpp->data_pattern_length;
				else pattern_length = remaining_length;
		
				memcpy(ucp,dpp->data_pattern,pattern_length);
				remaining_length -= pattern_length;
				ucp += pattern_length;
	    	}
		} else { // Just put the pattern at the beginning of the buffer once 
	    	if (dpp->data_pattern_length < (size_t)tdp->td_xfer_size) 
				pattern_length = dpp->data_pattern_length;
	    	else pattern_length = tdp->td_xfer_size;
	    	memcpy(wdp->wd_task.task_datap,dpp->data_pattern,pattern_length);
		}
    } else if (dpp->data_pattern_options & DP_LFPAT_PATTERN) {
		memset(wdp->wd_task.task_datap,0x00,tdp->td_xfer_size);
		dpp->data_pattern_length = sizeof(lfpat);
		fprintf(stderr,"LFPAT length is %d\n", (int)dpp->data_pattern_length);
		memset(wdp->wd_task.task_datap,0x00,tdp->td_xfer_size);
		remaining_length = tdp->td_xfer_size;
		ucp = (unsigned char *)wdp->wd_task.task_datap;
		while (remaining_length) { 
	    	if (dpp->data_pattern_length < remaining_length) 
				pattern_length = dpp->data_pattern_length;
	    	else pattern_length = remaining_length;
	    	memcpy(ucp,lfpat,pattern_length);
	    	remaining_length -= pattern_length;
	    	ucp += pattern_length;
		}
    } else if (dpp->data_pattern_options & DP_LTPAT_PATTERN) {
		memset(wdp->wd_task.task_datap,0x00,tdp->td_xfer_size);
		dpp->data_pattern_length = sizeof(ltpat);
		fprintf(stderr,"LTPAT length is %d\n", (int)dpp->data_pattern_length);
		memset(wdp->wd_task.task_datap,0x00,tdp->td_xfer_size);
		remaining_length = tdp->td_xfer_size;
		ucp = (unsigned char *)wdp->wd_task.task_datap;
		while (remaining_length) { 
	    	if (dpp->data_pattern_length < remaining_length) 
				pattern_length = dpp->data_pattern_length;
	    	else pattern_length = remaining_length;
	    	memcpy(ucp,ltpat,pattern_length);
	    	remaining_length -= pattern_length;
	    	ucp += pattern_length;
		}
    } else if (dpp->data_pattern_options & DP_CJTPAT_PATTERN) {
		memset(wdp->wd_task.task_datap,0x00,tdp->td_xfer_size);
		dpp->data_pattern_length = sizeof(cjtpat);
		fprintf(stderr,"CJTPAT length is %d\n", (int)dpp->data_pattern_length);
		memset(wdp->wd_task.task_datap,0x00,tdp->td_xfer_size);
		remaining_length = tdp->td_xfer_size;
		ucp = (unsigned char *)wdp->wd_task.task_datap;
		while (remaining_length) { 
	    	if (dpp->data_pattern_length < remaining_length) 
				pattern_length = dpp->data_pattern_length;
	    	else pattern_length = remaining_length;
	    	memcpy(ucp,cjtpat,pattern_length);
	    	remaining_length -= pattern_length;
	    	ucp += pattern_length;
		}
    } else if (dpp->data_pattern_options & DP_CRPAT_PATTERN) {
		memset(wdp->wd_task.task_datap,0x00,tdp->td_xfer_size);
		dpp->data_pattern_length = sizeof(crpat);
		fprintf(stderr,"CRPAT length is %d\n", (int)dpp->data_pattern_length);
		memset(wdp->wd_task.task_datap,0x00,tdp->td_xfer_size);
		remaining_length = tdp->td_xfer_size;
		ucp = (unsigned char *)wdp->wd_task.task_datap;
		while (remaining_length) { 
	    	if (dpp->data_pattern_length < remaining_length) 
				pattern_length = dpp->data_pattern_length;
	    	else pattern_length = remaining_length;
	    	memcpy(ucp,crpat,pattern_length);
	    	remaining_length -= pattern_length;
	    	ucp += pattern_length;
		}
    } else if (dpp->data_pattern_options & DP_CSPAT_PATTERN) {
		memset(wdp->wd_task.task_datap,0x00,tdp->td_xfer_size);
		dpp->data_pattern_length = sizeof(cspat);
		fprintf(stderr,"CSPAT length is %d\n", (int)dpp->data_pattern_length);
		memset(wdp->wd_task.task_datap,0x00,tdp->td_xfer_size);
		remaining_length = tdp->td_xfer_size;
		ucp = (unsigned char *)wdp->wd_task.task_datap;
		while (remaining_length) { 
	    	if (dpp->data_pattern_length < remaining_length) 
				pattern_length = dpp->data_pattern_length;
	    	else pattern_length = remaining_length;
	    	memcpy(ucp,cspat,pattern_length);
	    	remaining_length -= pattern_length;
	    	ucp += pattern_length;
		}
    } else { // Otherwise set the entire buffer to the character in "dpp->data_pattern"
		memset(wdp->wd_task.task_datap,*(dpp->data_pattern),tdp->td_xfer_size);
   	}
		
    return;
} // end of xdd_datapattern_buffer_init()
/*----------------------------------------------------------------------------*/
/* xdd_datapattern_fill() - This subroutine will fill the buffer with a 
 * specific pattern. 
 * This routine is called within the inner I/O loop for every I/O if the data
 * pattern changes from IO to IO
 */
void
xdd_datapattern_fill(worker_data_t *wdp) {
	target_data_t	*tdp;
	size_t  		j;					// random variables 
	uint64_t 		*posp;             	// Position Pointer 
	nclk_t			start_time;			// Used for calculating elapsed times of ops
	nclk_t			end_time;			// Used for calculating elapsed times of ops


	tdp = wdp->wd_tdp;
	/* Sequenced Data Pattern */
	if (tdp->td_dpp->data_pattern_options & DP_SEQUENCED_PATTERN) {
		nclk_now(&start_time);
		posp = (uint64_t *)wdp->wd_task.task_datap;
		for (j=0; j<(tdp->td_xfer_size/sizeof(wdp->wd_task.task_byte_offset)); j++) {
			*posp = wdp->wd_task.task_byte_offset + (j * sizeof(wdp->wd_task.task_byte_offset));
			*posp |= tdp->td_dpp->data_pattern_prefix_binary;
			if (tdp->td_dpp->data_pattern_options & DP_INVERSE_PATTERN)
				*posp ^= 0xffffffffffffffffLL; // 1's compliment of the pattern
			posp++;
		}
		nclk_now(&end_time);
// FIXME ????		wdp->wd_accumulated_pattern_fill_time = (end_time - start_time);
	}
} // End of xdd_datapattern_fill() 

 
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
