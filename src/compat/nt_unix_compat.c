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
/* These routines are used to provide the functionality of certain
 * UNIX routines that are not in the Windows NT/2000/XP/Vista/Win7 environment.
 */
#include "xdd.h"
/*-----------------------------------------------------------------*/
/* mlockall() - Lock user buffers and process pages into memory
 * Return values: 0 is good, -1 is bad. 
 * There is no equivalent function in Windows.
 */
int
mlockall(unsigned int flag) {
	return(0);
} /* end of mlockall() */
/*-----------------------------------------------------------------*/
/* mlock() - Lock user buffers pages into memory
 * Return values: 0 is good, -1 is bad. 
 * There is no equivalent function in Windows.
 */
int
mlock(unsigned char *bp, uint32_t size) {
	return(0);
} /* end of mlock() */
/*-----------------------------------------------------------------*/
/* munlock() - Free user buffers pages previously locked into memory
 * Return values: 0 is good, -1 is bad.
 * There is no equivalent function in Windows.
 */
int
munlock(unsigned char *bp, uint32_t size) {
	return(0);
} /* end of munlock() */
/*-----------------------------------------------------------------*/
/* sched_get_priority_max() - Return the maximum priority class that
 * a program can set itself to. This value is later used by 
 * the sched_setscheduler() routine to maximize the priority of this
 * program.
 */
unsigned long
sched_get_priority_max(int flag) {
	return(REALTIME_PRIORITY_CLASS);
} /* end of sched_get_priority_max() */
/*-----------------------------------------------------------------*/
/* sched_setscheduler() - This routine will set the priority class
 * of this program to the requested priority class.
 * This routine always returns 0 for successful operation.
 */
int
sched_setscheduler(int arg1, unsigned long arg2, struct sched_param *pp) {
	HANDLE  thread_handle;
	BOOL  status;
	LPVOID lpMsgBuf; /* Used for the error messages */
	thread_handle = GetCurrentThread();
	/* reset the priority to max max max */
	status = SetThreadPriority(thread_handle,THREAD_PRIORITY_TIME_CRITICAL);
	if (status == 0) { 
		fprintf(stderr,"SetThreadPriority: cannot reset process priority\n");
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
		fprintf(stderr,"Reason:%s",lpMsgBuf);
	}
	return(0);
} /* end of sched_setscheduler() */
/*-----------------------------------------------------------------*/
/* lseek64() - This routine will perform a seek operation within a
 * 64-bit address space. This routine maps to the SetFilePointer()
 * Windows routine. 
 */
int64_t
lseek64(unsigned long fd, int64_t offset, int flag) {
	LONG phi,plow;
	plow = (unsigned long)offset;
	phi = (unsigned long)(offset >> 32);
	SetFilePointer((void *)fd, plow, &phi, FILE_BEGIN);
	return(offset);
} /* end of lseek64() */
/*-----------------------------------------------------------------*/
/* fsync() - 
 * This routine has no Windows equivalent.
 */
int
fsync(int fd) {
	return(0);
} /* end of fsync() */
/*-----------------------------------------------------------------*/
/* getpagesize() - This function maps to the GetSystemInfo function
 * in Windows that returns a structure that contains the page size 
 * in bytes.
 */
int
getpagesize(void) {
	SYSTEM_INFO system_info;
	GetSystemInfo(&system_info);
	return(system_info.dwPageSize);
} /* end of getpagesize() */
/*-----------------------------------------------------------------*/
/* Return the wall clock time in milliseconds. Also, fill in the
 * tms time structure with kernel and user times. The GetThreadTimes
 * routine returns time values in 100-nanosecond increments. It is
 * converted to CLK_TCK increments which is milliseconds.
 */
clock_t
times( struct tms *tp) {
	HANDLE  thread_handle;
	BOOL  status;
	FILETIME kerneltime;
	FILETIME usertime;
	FILETIME createtime;
	FILETIME exittime;
	struct _timeb tstruct; 
	clock_t  wallclock;
	thread_handle = GetCurrentThread();
	status = GetThreadTimes(
				thread_handle,
				&createtime,
				&exittime,
				&kerneltime,
				&usertime);
	if (status == 0) {
		fprintf(stderr,"times: cannot get thread times\n");
		perror("Reason");
		return(0);
	}
	tp->tms_utime = (unsigned long)((__int64)((usertime.dwHighDateTime << 32) | usertime.dwLowDateTime)/10000);
	tp->tms_stime = (unsigned long)((__int64)((kerneltime.dwHighDateTime << 32) | kerneltime.dwLowDateTime)/10000);
    _ftime( &tstruct );
    wallclock = ((tstruct.time*1000)+tstruct.millitm);
	return(wallclock);
} /* end of times() */
/*-----------------------------------------------------------------*/
/* sleep() - This function maps to the Windows Sleep() function.
 */
void
sleep(int seconds) {
	Sleep(seconds*1000);
} /* end of sleep() */
/*-----------------------------------------------------------------*/
int
pthread_create(pthread_t *tp, void *thread_attr, void *thread_function, void *thread_arg)
{
	HANDLE thread_handle;
	thread_handle = CreateThread(NULL, 0, thread_function, thread_arg, 0,(unsigned long *)tp);
	if (thread_handle == FALSE) {
		fprintf(stderr,"pthread_create: Could not create target manager\n");
		perror("Reason");
		return(0);
	}
	tp->pthread = (pthread_t *)thread_handle;
	return(0);
} /* end of pthread_create() */
/*-----------------------------------------------------------------*/
/* This mutex is needed to circumvent a windows bug that has to
 * do with multiple threads creating multiple stream files and
 * clobbering each others stuff. 
 */
int32_t
ts_serializer_init(HANDLE *mp, char *mutex_name) {
	HANDLE mutex_handle;
	LPVOID lpMsgBuf; 
	mutex_handle = CreateMutex(NULL,FALSE, mutex_name);
	if (mutex_handle == 0) {
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
		fprintf(stderr,"ts_serializer_init: Could not create mutex\n");
		fprintf(stderr,"reason:%s",lpMsgBuf);
		*mp = 0;
		return(-1);
	}
	*mp = mutex_handle;
	WaitForSingleObject(mp,INFINITE); /* make sure we have ownership of this mutex */
	return(0);
}
/*-----------------------------------------------------------------*/
int32_t
pthread_mutex_init(pthread_mutex_t *mp, int n) {
	HANDLE mutex_handle;
	LPVOID lpMsgBuf;
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	sprintf(mp->name,"xddMutex%llx%",mp);
	mutex_handle = CreateMutex(&sa,FALSE,mp->name);
	if (mutex_handle == 0) {
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
		fprintf(stderr,"pthread_mutex_init: Could not create mutex %s\n",mp->name);
		fprintf(stderr,"reason:%s",lpMsgBuf);
		return(-1);
	}
	mp->mutex = mutex_handle;
	return(0);
} /* end of pthread_mutex_init() */
/*-----------------------------------------------------------------*/
void
pthread_mutex_lock(pthread_mutex_t *mp) {
	DWORD status;
	status = WaitForSingleObject(mp->mutex, INFINITE);
	if (status == WAIT_OBJECT_0)
		return;
	if (status == WAIT_ABANDONED)
		fprintf(stderr,"pthread_mutex_lock: wait returned with WAIT_ABANDONED\n");
	if (status == WAIT_TIMEOUT) 
		fprintf(stderr,"pthread_mutex_lock: wait returned with WAIT_TIMEOUT\n");
	return;
} /* end of pthread_mutex_lock() */
/*-----------------------------------------------------------------*/
void
pthread_mutex_unlock(pthread_mutex_t *mp) {
	DWORD status;
	LPVOID lpMsgBuf;
	status = ReleaseMutex(mp->mutex);
	if (status == 0) {
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
		fprintf(stderr,"pthread_mutex_unlock: Could not release mutex\n");
		fprintf(stderr,"reason:%s",lpMsgBuf);
	}
} /* end of pthread_mutex_unlock() */
/*-----------------------------------------------------------------*/
/* pthread_mutex_destroy() - pthread  
 * A return value of -1 is bad. Anything else is good.
 */
int
pthread_mutex_destroy(pthread_mutex_t *mp) {
	CloseHandle(mp->mutex);
	return(0);
} /* end of pthread_mutex_destroy() */
/*-----------------------------------------------------------------*/
/* semop() - nonzero return is bad. 
 * This routine is kind of weird. Semaphores in Unixland work
 * quite the opposite - they are non-signaled when less than
 * zero and signaled when equal to zero or greater. The NT
 * semaphore is non-signaled when equal to zero and signaled
 * when greater than zero. 
 * The way we use semaphores in xdd is as a synchronization barrier
 * for all the threads to reach before continuing. 
 * semop is called with a count "sp->sem_op" of -1 each time a thread is
 * supposed to wait at the barrier. If "sp->sem_op" is positive, then
 * it is time to set the semaphore to a signaled state thus releasing
 * all the waiting threads. It is important to note that if the sem_op is 
 * a -1, then the WaitForSingleObject() routine is called at which time
 * the  WaitForSingleObject() routine will first check the state of the 
 * of the semaphore: if the semaphore is 0 then the calling process is
 * suspended until the semaphore is raised to a signaled state (positive
 * number greater than 0). The semaphore is raised to a signaled state
 * by the calling routine when the sem_op is a positive number greater
 * than 0. The semaphore is then released and given this new value.
 * The important part is that after the semaphore is set to this positive
 * value (signaled), each wait function waiting on that semaphore will 
 * subtract 1 from the semaphore and then return to this routine which 
 * subsequently releases the associated thread.
 * 
 */
int
semop(HANDLE semid, struct sembuf *sp, int n) {
	long prevcount;
	DWORD status;
	LPVOID lpMsgBuf;
	if (sp->sem_op == -1) { 
		status = WaitForSingleObject(semid,INFINITE);
		if (status == -1) {
			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
			fprintf(stderr,"semop: Wait failed for semaphore semid 0x%x\n", semid);
			fprintf(stderr,"reason:%s",lpMsgBuf);
		}
	} else {
//fprintf(stderr,"Releasing sp=%x, semop=%d\n",sp,sp->sem_op);
		ReleaseSemaphore(semid,sp->sem_op,&prevcount);
//fprintf(stderr,"sp=%x, prevcount=%d\n",sp,prevcount);
	}
	return(0);
} /* end of semop() */
/*-----------------------------------------------------------------*/
/* semget() - Returns a semaphore identifier (semid).
 * A retturn value of -1 is bad, anything else is good.
 */
HANDLE
semget(unsigned int semid_base, unsigned int maxsem, unsigned int flags) {
	SECURITY_ATTRIBUTES sa; /* used to specify inheritance */
	HANDLE sh;    /* Semaphore handle */
	char sname[256];  /* name of semaphore */
	LPVOID lpMsgBuf;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	sprintf(sname,"xddseamphore%x",semid_base);
	sh = CreateSemaphore(&sa,0,MAX_TARGETS,sname);
	if (sh == NULL) {
			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
			fprintf(stderr,"semget: Could not create semaphore %s\n",sname);
			fprintf(stderr,"reason:%s",lpMsgBuf);
			return((void *)-1);
	}
	return(sh);
} /* end of semget() */
/*-----------------------------------------------------------------*/
/* semctl() - semaphore control 
 * A return value of -1 is bad. Anything else is good.
 */
int
semctl(HANDLE semid, unsigned int maxsem, unsigned int flags, unsigned short *zp) {
	if (flags == IPC_RMID)
		CloseHandle(semid);
	return(0);
} /* end of semctl() */
/*-----------------------------------------------------------------*/
/* sysmp() - assign a thread to a specific processor
 * This routine returns a -1 on error.
 * Otherwise, it returns the number of processors or the processor 
 * number that this thread is assigned to depending on the function
 * requested.
 */
int
sysmp(int op, int arg) {
	DWORD process_affinity; /* CPUs that this process is allowed to run on */
	DWORD system_affinity;  /* CPUs on this system */
	DWORD mask;  /* A mask used to count the processors on the system */
	BOOL status;  /* status of the GetProcessAffinityMask system call */
	HANDLE phandle; /* The handle for this process */
	HANDLE thandle; /* The handle for this thread */
	LPVOID lpMsgBuf;
	int i;
	int cpus;  /* The number of CPUs found on this system */
	phandle = GetCurrentProcess();
	thandle = GetCurrentThread();
	status = GetProcessAffinityMask(phandle, &process_affinity,&system_affinity);
	if (status == 0) {
		FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL);
		fprintf(stderr,"sysmp: Could not get process affinity mask\n");
		fprintf(stderr,"reason:%s",lpMsgBuf);
	}
	switch (op) {
	case MP_NPROCS:
		mask = 0x01;
		cpus = 0;
		for (i = 0; i < 31; i++) {
			if (mask & system_affinity) cpus++;
			mask <<= 1;
		}
		if (cpus == 0) {
			fprintf(stderr,"sysmp: MP_NPROCS: system affinity mask has no processors\n");
			cpus = 1;
		}
		return(cpus);
	case MP_MUSTRUN:
		mask = 0x01;
		cpus = 0;
		/* This loop will scan thru the system affinity mask bit by bit
		 * When the cpu number (cpus) is equal to the argument passed
		 * in by the calling routine, then the mask should have the
		 * correct value to use for the SetThreadAffinityMask() function.
		 */
		for (i = 0; i < 31; i++) {
			if (cpus == arg) break;
			if (mask & system_affinity) cpus++;
			mask <<= 1;
		}
		status = SetThreadAffinityMask(thandle,mask);
		if (status == 0) {
			FormatMessage( 
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM | 
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpMsgBuf,
				0,
				NULL);
			fprintf(stderr,"sysmp: Could not set process affinity mask 0x%08x\n",mask);
			fprintf(stderr,"reason:%s",lpMsgBuf);
			return(-1);
		}
		return(cpus); /* return the processor number that this thread was assigned to */
	default:
		fprintf(stderr,"sysmp: unknown operation: %d\n",op);
		return(-1);
	} /* end of SWITCH statement */
} /* end of sysmp() */
/*-----------------------------------------------------------------*/
/* getpid() - This function will return the current Windows Thread
 * ID which is similar to the Process ID in UNIX.
 */
int
getpid(void) {
	return((int)GetCurrentThreadId());
} /* end of getpid() */
/*-----------------------------------------------------------------*/
/* random() - This function maps to the Windows rand function.
 */
int
random(void) {
	return(rand());
} /* end of random() */
/*-----------------------------------------------------------------*/
/* initstate() - This function maps to windows srand function.
 */
void
initstate(int seed, char *state, int value) {
	srand((unsigned int)seed);
} /* end of random() */
 
 
 
 
 
