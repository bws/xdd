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
}

/**
 * Destroy resources associated with the semaphore
 */
inline static int xint_semaphore_destroy(xint_semaphore_t *sem)
{
    int rc1, rc2;

    assert(barrier->waiters == 0);
    rc1 = pthread_cond_destroy(&sem->cond);
    if (0 != rc1) {
	fprintf(stderr,
		"Error: Destroying barrier condvar count: %zd reason: %s\n",
		barrier->count, strerror(rc1));
    }
    assert(0 == rc1);
    
    rc2 = pthread_mutex_destroy(&sem->mutex);
    if (0 != rc2)
	fprintf(stderr,
		"Error: Destroying barrier mutex count: %zd reason: %s\n",
		barrier->count, strerror(rc2));
    assert(0 == rc2);
    return (rc1 + rc2);
}

/**
 * Increment the value of the semaphore and wake all of the semapohre's
 * waiters
 *
 * @return 0 on success, 1 on failure
 */
inline static int xint_semaphore_post(xint_semaphore_t *sem)
{
    assert(sem->waiters < sem->count);
    pthread_mutex_lock(&sem->mutex);

    // Increment the count
    sem->count++;

    // Decrement the number of waiters
    sem->waiters--;
    
    // Signal a waiter if any other waiters exist
    if (sem->waiters > 0) {
	pthread_cond_signal(&sem->cond);
    }

    pthread_mutex_unlock(&sem->mutex);
    return 0;
}

/**
 * @return the semaphore value
 */
inline static int xint_semaphore_getvalue(xint_semaphore_t *sem, int *valp)
{
    *valp = sem->value;
    return 0;
}

/**
 * Wait if the semaphore has a negative value
 */
inline static int xint_semaphore_wait(xint_semaphore_t *sem)
{
    assert(sem->waiters < sem->count);
    pthread_mutex_lock(&sem->mutex);
    sem->value--;
    if ( 0 > sem->value ) {
	sem->waiters++;
        pthread_cond_wait(&sem->cond, &sem->mutex);
    }

    pthread_mutex_unlock(&sem->mutex);
    return 0;
}

/**
 * If the semaphore has no current waiters, wait
 *
 * @return 0 if the lock was acquired, -1 if the lock was already held
 */
inline static int xint_semaphore_trywait(xint_semaphore_t *sem)
{
    int rc;
    assert(sem->waiters < sem->count);
    pthread_mutex_lock(&sem->mutex);
    if (0 == sem->count) {
	sem->waiters++;
        pthread_cond_wait(&sem->cond, &sem->mutex);
	rc = 0;
    } else {
	rc = -1;
    }
    pthread_mutex_unlock(&sem->mutex);
    return rc;
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
