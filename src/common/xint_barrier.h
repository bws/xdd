/*
 * XDD - a data movement and benchmarking toolkit
 *
 * Copyright (C) 1992-2013 I/O Performance, Inc.
 * Copyright (C) 1999-2006 Brian Paul
 * Copyright (C) 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright (C) 2009-2013 UT-Battelle, LLC
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License version 2, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

/* Pthread-based barrier adapted from Gallium */
#ifndef XINT_BARRIER_H
#define XINT_BARRIER_H

#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#define HAVE_XINT_BARRIER

typedef struct xint_barrier {
   size_t count;
   size_t waiters;
   pthread_mutex_t mutex;
   pthread_cond_t cond;
} xint_barrier_t;
 
inline static int xint_barrier_init(xint_barrier_t *barrier, size_t count)
{
    int rc = 0;
    barrier->count = count;
    barrier->waiters = 0;
    rc = pthread_mutex_init(&barrier->mutex, NULL);
    assert(0 == rc);
    rc = pthread_cond_init(&barrier->cond, NULL);
    assert(0 == rc);
    return rc;
}
 
inline static int xint_barrier_destroy(xint_barrier_t *barrier)
{
    //int rc = 0;

    // Have to remove the checking because the qthreads are sitting in a mutex
    // even during a successful completion
    //assert(barrier->waiters == 0);
    pthread_cond_destroy(&barrier->cond);
    //if (0 != rc)
//	fprintf(stderr,
//		"Error: Destroying barrier condvar count: %zd reason: %s\n",
//		barrier->count, strerror(rc));
    //assert(0 == rc);
    pthread_mutex_destroy(&barrier->mutex);
//    if (0 != rc)
//	fprintf(stderr,
//		"Error: Destroying barrier mutex count: %zd reason: %s\n",
//		barrier->count, strerror(rc));
    //assert(0 == rc);
    return 0;
}
 
inline static int xint_barrier_wait(xint_barrier_t *barrier)
{
    assert(barrier->waiters < barrier->count);
    pthread_mutex_lock(&barrier->mutex);    
    barrier->waiters++;

    if (barrier->waiters == barrier->count) {
	barrier->waiters = 0;
	pthread_cond_broadcast(&barrier->cond);
    } else {
        pthread_cond_wait(&barrier->cond, &barrier->mutex);
    }

    pthread_mutex_unlock(&barrier->mutex);
    return 0;
}

#endif
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
