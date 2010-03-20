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

/** These barriers are used for thread synchronization */
struct xdd_barrier {
	struct xdd_barrier 	*prev; 			// Previous barrier in the chain 
	struct xdd_barrier 	*next; 			// Next barrier in chain 
	int32_t				initialized; 	// Indicates that this semaphore has been initialized
	char				name[256]; 		// This is the ASCII name of the barrier
	int32_t				counter; 		// Couter used to keep track of how many threads have entered the barrier
	int32_t				threads; 		/// The number of threads that need to enter this barrier before openning
#ifdef WIN32
	HANDLE				sem;  			// The semaphore Object
#else
#ifdef SYSV_SEMAPHORES
	int32_t				sem;			// SystemV Semaphore ID
	int32_t				semid_base; 	// The unique base name of this SysV semaphore
#else
	pthread_barrier_t	pbar;			// The PThreads Barrier 
#endif
#endif
	pthread_mutex_t 	mutex;  		// Locking Mutex for access to the semaphore chain
};
typedef struct xdd_barrier xdd_barrier_t;

/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 * End:
 *
 * vim: ts=8 sts=8 sw=8 noexpandtab
 */

 
