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
 * @file barrier.h
 * Defines Thread barrier data structures
 */

/** These barriers are used for thread synchronization */
struct xdd_barrier {
	struct xdd_barrier *prev; /**< Previous barrier in the chain */
	struct xdd_barrier *next; /**< Next barrier in chain */
	int32_t  initialized; /**< TRUE means this barrier has been initialized */
	char  name[256]; /**< Undocumented */
	int32_t  counter; /**< counter variable */
	int32_t  threads; /**< number of threads participating */
	int32_t  semid_base; /**< Semaphore ID base */
#ifdef WIN32
	HANDLE  semid;  /**< id for this sempahore */
#else
	int32_t  semid;  /**< id for this semaphore */
#endif
	pthread_mutex_t mutex;  /**< locking mutex */
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

 
