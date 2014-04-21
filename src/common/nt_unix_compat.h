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

/** Map pthread structures to their windows equivalents */
struct pthread {
	HANDLE pthread;
};
typedef struct pthread pthread_t;

/** Map pthread_mutex structures to their windows equivalents */
struct pthread_mutex {
	HANDLE mutex;
	char name[MAX_PATH];
};
typedef struct pthread_mutex pthread_mutex_t;

/* Map semaphore structure to the windows equivalent */
struct sembuf {
	int             sem_op;  /* operation */
	unsigned long   sem_flg; /* flags */
	int             sem_num; /* number */
};

/* This section contains a variety of definintions that map unix-isms to Windows for compatibility */
typedef unsigned    int     uint32_t;
typedef unsigned    long    ulong_t;
typedef             int     int32_t;
typedef unsigned    __int64 uint64_t;
typedef             __int64 int64_t;
typedef unsigned    short   uint16_t;
typedef             short   int16_t;
typedef unsigned    int    in_addr_t; /**< An IP number */
typedef unsigned short in_port_t; /**< A port number */
typedef SOCKET  sd_t;  /**< A socket descriptor */

/* The following structures do not exist in the Windows header files so they
 * have been recreated here for use with their equivalent functions that live
 * in nt_unix_compat.c as part of the xdd distribution 
 */

/** UNIX tmstruct for Win32 */
struct tms {
	unsigned long tms_utime;
	unsigned long tms_stime;
};

/** UNIX timezone struct for Win32 */
struct timezone {
	unsigned int tz;
};

/** UNIX Scheduling parameter for Win32 */
struct sched_param {
	unsigned int sched_priority;
};

#define SCHED_RR    1   /**< Round Robin Scheduling */
#define SCHED_FIFO  2   /**< First In First Out Scheduling */

#define MCL_CURRENT 1   /**< Lock Current Memory pages  */
#define MCL_FUTURE  2   /**< Lock Future memory pages */

#define MP_MUSTRUN  1   /**< Assign this thread to a specific processor */
#define MP_NPROCS   2   /**< return the number of processors on the system */

#define IPC_RMID    -3  /**< remove  a semaphore */
#define EIDRM       43  /**< errno value for Identifer Removed */
#define SETALL 1  /**< used for Semaphore ops */
#define IPC_CREAT 01000 /**< Used to create Semaphores */

/* These are function calls that do not exist in the Windows CLibrary 
 * so they have been recreated in the nt_unix_compat.c file and their
 * prototypes live here 
 */

/**
 * Lock all mapped pages of memory into RAM.
 *
 * @param flag indicate whether to lock current or future pages
 * @return 0 on success, -1 on error with corresponding errno
 */
int mlockall(unsigned int flag);

/**
 * Lock pages starting at bp and ending at bp + size into memory
 *
 * @param bp beginning address to lock
 * @param size number of addresses following bp to lock
 * @return 0 on success, -1 on error with corresponding errno
 */
int mlock( unsigned char *bp, uint32_t size);

/**
 * Unlock pages starting at bp and ending at bp + size into memory
 *
 * @param bp beginning address to unlock
 * @param size number of addresses following bp to unlock
 * @return 0 on success, -1 on error with corresponding errno
 */
int munlock( unsigned char *bp, uint32_t size);

/**
 * @param flag Scheduling algorithm plicies
 * @return The maximum priority value, -1 on error with corresponding errno
 */
unsigned long sched_get_priority_max(int flag);

/**
 * Set the scheduling policy for a process
 *
 * @param arg1 The process to set the scheduler for.  0 for this process.
 * @param arg2 The scheduling policy
 * @param pp Scheduling parameter struct
 * @return 0 on success, -1 on error with corresponding errno
 */
int sched_setscheduler(int arg1, unsigned long arg2, struct sched_param *pp);

/**
 * Reposition the 64-bit file pointer
 *
 * @param fd file descriptor
 * @param offset to seek to relative to the file begin, current position, or end
 * @param whence Relative seek begin position (start, current, or end)
 * @return The resulting file pointer location, -1 on error with corresponding
 *  errno
 */
int64_t lseek64(unsigned long fd, int64_t offset, int flag);

/**
 * Transfers all modified in-memory file data to the disk device
 *  (incl. metadata)
 * @param fd File descriptor
 * @return 0 on success, -1 on error with corresponding errno
 */
int fsync(int fd);

/** @return The system page size */
int getpagesize(void);

/**
 * Store the current time in struct tms
 *
 * @param tp struct tms to fill out with time data
 * @return Non-zero on success, -1 on error with corresponding errno
 */
clock_t times( struct tms *tp);

/**
 * Set timeval struct to the current time
 *
 * @param tvp timeval struct to store the current time
 * @param tzp Deprecated.  Supply as 0 or NULL.
 */
void gettimeofday(struct timeval *tvp, struct timezone *tzp);

/**
 * Sleep until seconds have elapsed
 *
 * @param seconds Number of seconds until process awakens 
 */
void sleep(int seconds);

/** FIXME - No doc provided */
int sysmp(int op, int arg);

/**
 * Initialize and start thread
 *
 * @param tp pointer to allocated pthread_t structure
 * @param thread_attr pthread_attr_t thread attributes?
 * @param thread_function function to invoke with thread (wrong type)
 * @param thread_arg data passed into thread function
 * @return 0 on success, -1 on error with corresponding errno
 */
int pthread_create(pthread_t *tp, void *thread_attr, void *thread_function, void *thread_arg);

/**
 * Initialize mutex mp.
 * @param mp mutex pointer
 * @param n FIXME - huh?
 * @return 0 on success, -1 on error with corresponding errno
 */
int32_t pthread_mutex_init(pthread_mutex_t *mp, int n);

/**
 * @param mp mutex pointer
 * @return 0 on success, -1 on error with corresponding errno
 */
int32_t pthread_mutex_destroy(pthread_mutex_t *mp);

/**
 * Lock mutex mp. Call blocks until mutex is acquired.
 * @param mp mutex pointer
 */
void pthread_mutex_lock(pthread_mutex_t *mp);

/**
 * Unlock mutex mp.
 * @param mp mutex pointer
 */
void pthread_mutex_unlock(pthread_mutex_t *mp);

/**
 * Performs operation on semaphore
 * @param semid semaphore to modify
 * @param sp list of operations to be performed
 * @param n number of operations
 * @return 0 on success, -1 on error with corresponding errno
 */
int semop(HANDLE semid, struct sembuf *sp, int n);

/**
 * @param semid_base The key for the semaphore set
 * @param maxsem number of semaphores to retrive/create
 * @param flags Semaphore retrieval flags
 * @return Requested semaphore set
 */
HANDLE semget(unsigned int semid_base, unsigned int maxsem, unsigned int flags);

/**
 * @param semid Semaphore set to operate on
 * @param maxsem
 * @param flags
 * @param zp
 * @return 0 on success, -1 on error with corresponding errno
 */
int semctl(HANDLE semid, unsigned int maxsem, unsigned int flags, unsigned short *zp);

/** @return the process id */
int getpid(void);

/**
 * @param c numeric string to convert to long long
 * @return Long long value represented in the string argument
 */
int64_t atoll(char *c);

/**
 * Initialization state array for latter calls to random()
 * @param seed random seed
 * @param state state array
 * @param value the size of the state array
 */
void initstate(int seed, char *state, int value);

/**
 * FIXME - No doc provided.
 * @return Probably returns a random number in some range.
 */
int xdd_random_int(void);
 
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
