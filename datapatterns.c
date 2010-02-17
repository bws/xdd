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
 *       Steve Hodson, DoE/ORNL
 *       Steve Poole, DoE/ORNL
 *       Bradly Settlemyer, DoE/ORNL
 *       Russell Cattelan, Digital Elves
 *       Alex Elder
 * Funding and resources provided by:
 * Oak Ridge National Labs, Department of Energy and Department of Defense
 *  Extreme Scale Systems Center ( ESSC ) http://www.csm.ornl.gov/essc/
 *  and the wonderful people at I/O Performance, Inc.
 */
/*
 * This file contains the subroutines that initialize various data patterns
 * used by XDD
 */
#include "xdd.h"
#include "datapatterns.h"

/*----------------------------------------------------------------------------*/
/* xdd_pattern_buffer() - init the I/O buffer with the appropriate pattern
 * This routine will put the requested pattern in the rw buffer.
 */
void
xdd_datapattern_buffer_init(ptds_t *p) {
	int32_t i;
	int32_t pattern_length; // Length of the pattern
	int32_t remaining_length; // Length of the space in the pattern buffer
	unsigned char    *ucp;          // Pointer to an unsigned char type, duhhhh
	uint32_t *lp;			// pointer to a pattern


	if (p->target_options & TO_RANDOM_PATTERN) { // A nice random pattern
			lp = (uint32_t *)p->rwbuf;
			xgp->random_initialized = 0;
            /* Set each four-byte field in the I/O buffer to a random integer */
			for(i = 0; i < (int32_t)(p->iosize / sizeof(int32_t)); i++ ) {
				*lp=xdd_random_int();
				lp++;
			}
	} else if ((p->target_options & TO_ASCII_PATTERN) ||
	           (p->target_options & TO_HEX_PATTERN)) { // put the pattern that is in the pattern buffer into the io buffer
			// Clear out the buffer before putting in the string so there are no strange characters in it.
			memset(p->rwbuf,'\0',p->iosize);
			if (p->target_options & TO_REPLICATE_PATTERN) { // Replicate the pattern throughout the buffer
				ucp = (unsigned char *)p->rwbuf;
				remaining_length = p->iosize;
				while (remaining_length) { 
					if (p->data_pattern_length < remaining_length) 
						pattern_length = p->data_pattern_length;
					else pattern_length = remaining_length;

					memcpy(ucp,p->data_pattern,pattern_length);
					remaining_length -= pattern_length;
					ucp += pattern_length;
				}
			} else { // Just put the pattern at the beginning of the buffer once 
				if (p->data_pattern_length < p->iosize) 
					 pattern_length = p->data_pattern_length;
				else pattern_length = p->iosize;
				memcpy(p->rwbuf,p->data_pattern,pattern_length);
			}
	} else if (p->target_options & TO_LFPAT_PATTERN) {
		memset(p->rwbuf,0x00,p->iosize);
                p->data_pattern_length = sizeof(lfpat);
                fprintf(stderr,"LFPAT length is %d\n", (int)p->data_pattern_length);
		memset(p->rwbuf,0x00,p->iosize);
		remaining_length = p->iosize;
		ucp = (unsigned char *)p->rwbuf;
		while (remaining_length) { 
			if (p->data_pattern_length < remaining_length) 
				pattern_length = p->data_pattern_length;
			else pattern_length = remaining_length;
			memcpy(ucp,lfpat,pattern_length);
			remaining_length -= pattern_length;
			ucp += pattern_length;
		}
	} else if (p->target_options & TO_LTPAT_PATTERN) {
		memset(p->rwbuf,0x00,p->iosize);
                p->data_pattern_length = sizeof(ltpat);
                fprintf(stderr,"LTPAT length is %d\n", (int)p->data_pattern_length);
		memset(p->rwbuf,0x00,p->iosize);
		remaining_length = p->iosize;
		ucp = (unsigned char *)p->rwbuf;
		while (remaining_length) { 
			if (p->data_pattern_length < remaining_length) 
				pattern_length = p->data_pattern_length;
			else pattern_length = remaining_length;
			memcpy(ucp,ltpat,pattern_length);
			remaining_length -= pattern_length;
			ucp += pattern_length;
		}
	} else if (p->target_options & TO_CJTPAT_PATTERN) {
		memset(p->rwbuf,0x00,p->iosize);
                p->data_pattern_length = sizeof(cjtpat);
                fprintf(stderr,"CJTPAT length is %d\n", (int)p->data_pattern_length);
		memset(p->rwbuf,0x00,p->iosize);
		remaining_length = p->iosize;
		ucp = (unsigned char *)p->rwbuf;
		while (remaining_length) { 
			if (p->data_pattern_length < remaining_length) 
				pattern_length = p->data_pattern_length;
			else pattern_length = remaining_length;
			memcpy(ucp,cjtpat,pattern_length);
			remaining_length -= pattern_length;
			ucp += pattern_length;
		}
	} else if (p->target_options & TO_CRPAT_PATTERN) {
		memset(p->rwbuf,0x00,p->iosize);
                p->data_pattern_length = sizeof(crpat);
                fprintf(stderr,"CRPAT length is %d\n", (int)p->data_pattern_length);
		memset(p->rwbuf,0x00,p->iosize);
		remaining_length = p->iosize;
		ucp = (unsigned char *)p->rwbuf;
		while (remaining_length) { 
			if (p->data_pattern_length < remaining_length) 
				pattern_length = p->data_pattern_length;
			else pattern_length = remaining_length;
			memcpy(ucp,crpat,pattern_length);
			remaining_length -= pattern_length;
			ucp += pattern_length;
		}
	} else if (p->target_options & TO_CSPAT_PATTERN) {
		memset(p->rwbuf,0x00,p->iosize);
                p->data_pattern_length = sizeof(cspat);
                fprintf(stderr,"CSPAT length is %d\n", (int)p->data_pattern_length);
		memset(p->rwbuf,0x00,p->iosize);
		remaining_length = p->iosize;
		ucp = (unsigned char *)p->rwbuf;
		while (remaining_length) { 
			if (p->data_pattern_length < remaining_length) 
				pattern_length = p->data_pattern_length;
			else pattern_length = remaining_length;
			memcpy(ucp,cspat,pattern_length);
			remaining_length -= pattern_length;
			ucp += pattern_length;
		}
    } else { // Otherwise set the entire buffer to the character in "data_pattern"
                memset(p->rwbuf,*(p->data_pattern),p->iosize);
	}
		
	return;
} // end of xdd_datapattern_buffer_init()
