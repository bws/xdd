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
#include "target_offset_table.h"
#include <assert.h>
#include <stdlib.h>
#include "config.h"
#include "xint.h"

#define DEFAULT_TOT_MULTIPLIER 20

/**
 * Initialize the target offset table
 */
int tot_init(tot_t** table, size_t queue_depth, size_t num_reqs)
{
    int num_entries, rc, i;
    tot_t *tp;
    
    // Preconditions check
    assert(NULL != table);
    assert(num_reqs > 0);
    assert(queue_depth > 0);

    // Scale the TOT with respect to queue depth and the number of requests
    // There is nothing magical in this logic, its just a guess that these
    // values are large enough, its no big deal to scale them differently
    // as the understanding of this table improves
    num_entries = queue_depth * DEFAULT_TOT_MULTIPLIER;
    if (queue_depth > num_reqs) {
        num_entries = queue_depth;
    }
    else if ( (num_reqs/queue_depth) > (DEFAULT_TOT_MULTIPLIER*queue_depth*1000)) {
        num_entries *= 30;
    }
    
    // Initialize the memory
#if HAVE_VALLOC
    *table = valloc(num_entries * sizeof(tot_entry_t));
    rc = (NULL != table);
#elif HAVE_POSIX_MEMALIGN
    rc = posix_memalign((void**)table,
			sysconf(_SC_PAGESIZE),
			num_entries * sizeof(tot_entry_t));
#else
    *table = malloc(num_entries * sizeof(tot_entry_t));
    rc = (NULL != table);
#endif
    if (0 != rc)
	return -1;
    

    // Initialize all the table entries
    tp = *table;
    tp->tot_entries = num_entries;
    for (i = 0; i < tp->tot_entries; i++) {
	// Initialize the condvar
        rc = pthread_cond_init(&tp->tot_entry[i].tot_condition, 0);
	if (0 != rc) {
	    tp->tot_entries = i;
	    break;
	}

	// Initialize the mutex
        rc = pthread_mutex_init(&tp->tot_entry[i].tot_mutex, 0);
        if (0 != rc) {
	    pthread_cond_destroy(&tp->tot_entry[i].tot_condition);
	    tp->tot_entries = i;
	    break;
        }
        tp->tot_entry[i].is_released = 0;
        tp->tot_entry[i].tot_op_number = -1;
        tp->tot_entry[i].tot_byte_location = -1;
        tp->tot_entry[i].tot_io_size = 0;
    }

    // Perform cleanup if inititalization did not complete successfully
    if (0 != rc)
	tot_destroy(tp);
    
    return rc;
}

int tot_destroy(tot_t* tp)
{
    int i, status;

    // Preconditions check
    assert(NULL != tp);
    
    // Release the semaphores in the table
    status = 0;
    for (i = 0; i < tp->tot_entries; i++) {
        status += pthread_cond_destroy(&tp->tot_entry[i].tot_condition);
        status += pthread_mutex_destroy(&tp->tot_entry[i].tot_mutex);
    } 

    // Free the memory
    free(tp);

    return status;
}

int tot_update(tot_t* table,
	       int64_t req_number,
	       int qthread_number,
	       int64_t offset,
	       int32_t size)
{
    int rc = 0;
    int idx;
    tot_entry_t* tep;

    // Check preconditions
    assert(NULL != table);
    assert(0 <= req_number);
    assert(0 <= qthread_number);

    
    idx = req_number % table->tot_entries;
    tep = &(table->tot_entry[idx]);

    // Lock entry
    pthread_mutex_lock(&tep->tot_mutex);

    // Do not update if a newer entry is using this slot
    if (tep->tot_op_number >= req_number) {
	rc = -1;
	fprintf(xgp->errout,
		"%s: tot_update: QThread %d: "
		"WARNING: TOT Collision at entry %d, op number %"PRId64", "
		"byte location is %"PRId64" [block %"PRId64"], "
		"my current op number is %"PRId64", "
		"my byte location is %"PRId64" [block %"PRId64"] "
		"last updated by qthread %d\n",
		xgp->progname, qthread_number,
		idx, tep->tot_op_number,
		tep->tot_byte_location, tep->tot_byte_location/tep->tot_io_size,
		req_number,
		offset, offset/size,
		tep->tot_update_qthread_number);
    }
    else {
	nclk_now(&tep->tot_update_ts);
	tep->tot_update_qthread_number = qthread_number;
	tep->tot_op_number = req_number;
	tep->tot_byte_location = offset;
	tep->tot_io_size = size;
    }

    // Unlock entry
    pthread_mutex_unlock(&tep->tot_mutex);
    return rc;
}
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
