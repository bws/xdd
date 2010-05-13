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

// Barrier Naming Convention: 
//    (1) The first part of the name is the barrier "owner" or thread that initializes the barrier
//    (2) The second part of the name is the other [dependant] thread that will use this barrier
//    (3) The third part of the name is a descriptive word about the function of this barrier
//    (4) The fourth part of the name is the word "barrier" to indicate what it is
// Many barriers come in pairs to circumvent a race condition that occurs in many implementations
// of the underlying semaphore kernel code that causes barriers to "fall through" prematurely.
// Hence these barrier pairs have an "index" value associated with them that contains the same name.
// The barrier index is normally toggled between 0 and 1 by the barrier "owner".
// It is important to use the xdd_init_barrier() subroutine to initialize all barriers within XDD
// because it keeps track of all initialized barriers so that they can be properly shut down
// upon any exit from XDD. This prevents an unwanted accumulation of zombie semaphores lying about.
//
// About the Barrier Chain
// The barrier chain is used to keep track of all barriers that are created so that we can easily scan
// through the list during termination to make sure all of them have been removed properly.
// Barriers are added to the chain when they are initialized in xdd_barrier_init(). 
// Barriers are removed from the chain when they are destrroyed in xdd_barrier_destroy().
// The xdd_global_data structure contains a pointer to the first barrier on the chain, the last barrier
// on the chain, and the number of barriers on the chain at any given time.
// Each barrier structure contains a pointer to the barrier before it (the *prev* member) and a pointer
// to the barrier after it (the *next* member) in the chain. 
// At any given time the chain is a circular doubly-linked list that looks like so:
//                           (the last barrier)
//                            ^
//xgp->barrier_chain_first>+--|--------------+
//                         | prev            |
//                         | Barrier A       |
//                         |         next    |
//                         +-----------|-----+
//                            ^        V
//                         +--|--------------+
//                         | prev            |
//                         | Barrier B       |
//                         |         next    |
//                         +-----------|-----+
//                            ^        V
//                         +--|--------------+
//                         | prev            |
//                         | Barrier C       |
//                         |         next    |
//                         +-----------|-----+
//                            ^        V
//                         +--|--------------+
//                         | prev            |
//                         | Barrier D       |
//                         |         next    |
//                         +-----------|-----+
//                            ^        V
//xgp->barrier_chain_last->+--|--------------+
//                         | prev            |
//                         | Barrier E       |
//                         |         next    |
//                         +-----------|-----+
//                                     V
//                                (the first barrier)
// 
// Barriers are always added to the end of the chain.
// Barriers can be removed from the chain in any order.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// During normal execution a barrier will have some number of "occupants" or 
// threads that are waiting at a particular barrier for other threads to arrive.
// The occupants waiting in a barrier are of four different types:
//      - Target Thread
//      - QThread
//      - Support Thread like Results Manager, Heartbeat, Restart, or Interactive
//      - XDD Main - the main parent thread
// Each barrier will anchor a doubly linked circular list of "occupant" structures
// that contain information about the thread waiting on that barrier.
// The "occupant" structure contains the occupant's type, a pointer to a 
// PTDS if the occupant is a Target Thread or QThread, and previous/next pointers
// to the occupants in front or behind any given occupant structure. If the
// occupant is the only one on the occupant chain then it simply points to itself.
// Each time a thread enters a barrier, the pointer to the occupant structure passed 
// in as an argument to xdd_barrier() is added to the end of the occupant chain.
// Similarly, when the barrier is released, the occupant chain is cleared by the
// thread that caused the barrier to open. 
//
// The reason for the "occupant" chain is to be able to figure out at any
// given time if there are threads of any sort *stuck* in a barrier so that
// something might be done about releasing them without necessarily terminating
// the XDD run. This is useful for copy (aka end-to-end) operations that might
// have a stalled QThread that needs to be terminated. 
//
// The "occupant" chain is very similar to the barrier chain in that it is
// a circular doubly-linked list of "occupant" structures. Each barrier
// contains an "anchor" for its occupant chain that consists of a pair of
// pointers to the first and last occupant structure on the chain. New occupant
// structures are always added to the end of the chain. Occupant structures
// can be removed from anywhere in the chain but under normal operating conditions
// when the final occupant enters the barrier, the owner of the barrier will 
// simply remove the first/last pointers to the chain effectively taking all the
// occupants off the chain at once (rather than removing them individually).
// The "counter" member of the barrier structure indicates the number of occupants
// in the barrier at any given time. 
// The "threads" member of the barrier structure indicates the number of occupants 
// that must enter the barrier before all the occupants are released.
//
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The occupant structure
// When the occupant type is either TARGET or QTHREAD then the occupant_ptds member
// points (or should point) to the PTDS for that particular thread.
// The occupant_name should always point to a character string that indicates
// the name of the occupant. For example, if the occupant_type is SUPPORT then 
// the occupant_name will be "results_manager" or "heartbeat" or "restart". 
// If the occupant_type is a QTHREAD then the occupant_name will be "qthread_#_target_#"
// An occupant_type of TARGET will have an occupant_name of "target_#". 
// Finally, an occupant_type of MAIN will have an occupant_name of "main".
//
struct xdd_occupant {
	struct		xdd_occupant	*prev_occupant;	// Previous occupant on the chain
	struct		xdd_occupant	*next_occupant;	// Next occupant on the chain
	uint64_t				 	occupant_type;	// Bitfield that indicates the type of occupant
	char						*occupant_name;	// Pointer to a character string that is the name of this occupant
	struct		ptds			*occupant_ptds;	// Pointer to a PTDS if the occupant_type is a Target or QThread
	pclk_t						entry_time;		// Time stamp of when this occupant entered a barrier - filled in by xdd_barrier()
	pclk_t						exit_time;		// Time stamp of when this occupant was released from a barrier - filled in by xdd_barrier()
};
typedef struct xdd_occupant xdd_occupant_t;
#define XDD_OCCUPANT_TYPE_TARGET		0x0000000000000001ULL	// Occupant is a Target Thread
#define XDD_OCCUPANT_TYPE_QTHREAD		0x0000000000000002ULL	// Occupant is a QThread
#define XDD_OCCUPANT_TYPE_SUPPORT		0x0000000000000004ULL	// Occupant is a Support Thread: Results Manager, Heartbeat, Restart, Interactive
#define XDD_OCCUPANT_TYPE_MAIN			0x0000000000000008ULL	// Occupant is the XDD Main parent thread
#define XDD_OCCUPANT_TYPE_CLEANUP		0x0000000000000010ULL	// Occupant is a Target or QThread Cleanup function

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// The XDD barrier structure
//
#define XDD_BARRIER_NAME_LENGTH			64	// not to exceed this many characters in length
#define	XDD_BARRIER_FLAG_INITIALIZED	0x0000000000000001ULL	// Indicates that this barrier has been initialized
struct xdd_barrier {
	struct 	xdd_barrier 	*prev_barrier; 	// Previous barrier in the chain 
	struct 	xdd_barrier 	*next_barrier; 	// Next barrier in chain 
	struct	xdd_occupant	*first_occupant;// First Occupant in the chain
	struct	xdd_occupant	*last_occupant;	// Last Occupant in the chain
	uint32_t				flags; 			// State indicators and the like
	char					name[XDD_BARRIER_NAME_LENGTH]; 	// This is the ASCII name of the barrier
	int32_t					counter; 		// Couter used to keep track of how many threads have entered the barrier
	int32_t					threads; 		/// The number of threads that need to enter this barrier before occupants are released
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
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */

 
