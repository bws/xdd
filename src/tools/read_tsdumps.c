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
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <libgen.h>

#define XDDMAIN
#include <xint.h>

/* make sure MAX is defined */
#ifndef MAX
#define MAX(a,b) (a > b ? a : b)
#endif
#ifndef MIN
#define MIN(a,b) (a < b ? a : b)
#endif
/* converts xdd nclk time to seconds, given p (nclk value) */
#define nclk2sec(p) ((double)p/1.0e9)
/* Bytes Per Unit... don't set to 0.  1000000 is MB/s */
#define BPU (1000000.0)

/* returns 1 if x is expected operation... 0 if not */
#define READ_OP(x)  (x==TASK_OP_TYPE_READ||x==SO_OP_READ)
#define WRITE_OP(x) (x==TASK_OP_TYPE_WRITE||x==SO_OP_WRITE||x==SO_OP_WRITE_VERIFY)
#define NO_OP(x)    (x==TASK_OP_TYPE_NOOP||x==SO_OP_NOOP)
#define EOF_OP(x)   (x==TASK_OP_TYPE_EOF||x==SO_OP_EOF)

/* key offsets in xdd_ts_tte_t for the sorting function */
#define DISK_START  offsetof(xdd_ts_tte_t, tte_disk_start)
#define DISK_END    offsetof(xdd_ts_tte_t, tte_disk_end)
#define DISK_XFER   offsetof(xdd_ts_tte_t, tte_disk_xfer_size)
#define NET_START   offsetof(xdd_ts_tte_t, tte_net_start)
#define NET_END     offsetof(xdd_ts_tte_t, tte_net_end)
#define NET_XFER    offsetof(xdd_ts_tte_t, tte_net_xfer_size)

/* magic number to make sure we can read the file */
#define BIN_MAGIC_NUMBER 0xDEADBEEF

#define MAX_WORKER_THREADS 1024
int thread_id_src[MAX_WORKER_THREADS];
int thread_id_dst[MAX_WORKER_THREADS];
int total_threads_src = 0;
int total_threads_dst = 0;
double op_mix = 0.0;
nclk_t src_start_norm,dst_start_norm;

/* output file, if specified */
#define OUTFILENAME_LEN 512
char outfilebase[OUTFILENAME_LEN];
char kernfilename[OUTFILENAME_LEN];
char *srcfilename, *dstfilename;

int kernel_trace = 0;
/* number of seconds in the sliding window */
int window_size;

/* normalize timestamps */
void normalize_time(xdd_ts_header_t *src, nclk_t *start_norm);
/* custom quick sort for the xdd_ts_tte_t array */
void tte_qsort(xdd_ts_tte_t **list, size_t key_offset, int64_t left, int64_t right);
/* sorts the timestamp entries by completion time */
void sort_by_time(xdd_ts_header_t *tsdata, size_t op_offset, xdd_ts_tte_t ***op_sorted);
/* write all the outfiles */
void write_outfile(xdd_ts_header_t *src, xdd_ts_header_t *dst, xdd_ts_tte_t **read_op,
	xdd_ts_tte_t **send_op, xdd_ts_tte_t **recv_op, xdd_ts_tte_t **write_op);
/* read, check, and store the src and dst file data */
int xdd_readfile(char *filename, xdd_ts_header_t **tsdata, size_t *tsdata_size);
/* Write an XDD binary timestamp structure to file */
int xdd_writefile(char *filename, xdd_ts_header_t *tsdata, size_t tsdata_size);
/* parse command line options */
int getoptions(int argc, char **argv);
/* print command line usage */
void printusage(char *progname);
/* get total threads, thread pids, operation mix (%read,%write) */
void xdd_getthreads(xdd_ts_header_t *tsdata, int *total_threads, int thread_id[], double *op_mix);

int
matchadd_kernel_events(int issource, int nthreads, int thread_id[], char *filespec, xdd_ts_header_t *xdd_data);


int main(int argc, char **argv) {

	int fn,argnum,retval;
        size_t tsdata_size;
        char *iotrace_data_dir, *current_work_dir;
	/* tsdumps for the source and destination sides */
	xdd_ts_header_t *src = NULL;
	xdd_ts_header_t *dst = NULL;
	xdd_ts_header_t *tsdata = NULL;
	/* timestamp entries sorted by ending time */
	xdd_ts_tte_t **read_op = NULL;
	xdd_ts_tte_t **send_op = NULL;
	xdd_ts_tte_t **recv_op = NULL;
	xdd_ts_tte_t **write_op = NULL;

	/* get command line options */
	argnum = getoptions(argc, argv);
	fn = MAX(argnum,1);

	/* get the src or dst data structs */
	retval = xdd_readfile(argv[fn],&tsdata,&tsdata_size);
	if (retval == 0) {
  	  fprintf(stderr,"xdd_readfile() failed...reading 1st xdd timestamp file: %s ...exiting.\n",argv[fn]);
	  exit(1);
	}
        /* determine whether this is the source or destination file */
        /* if only 1 file given, then source==read, destination==write */
        if (READ_OP(tsdata->tsh_tte[0].tte_op_type)) { 
           src = tsdata;
           srcfilename = argv[fn];
	}
        else
        if (WRITE_OP(tsdata->tsh_tte[0].tte_op_type)){
	  dst = tsdata;
          dstfilename = argv[fn];
	}
        else {
           fprintf(stderr,"Timestamp dump had invalid operation in tte[0]: %s\n",argv[fn]);
           exit(1);
        }
        /* if 2nd file specified, this must be an e2e pair (src,dst) */
        if ( argv[fn+1] != NULL ) {
          /* get the src or dst data structs */
          retval = xdd_readfile(argv[fn+1],&tsdata,&tsdata_size);
	  if (retval == 0) {
	    fprintf(stderr,"xdd_readfile() failed...reading 2nd xdd timestamp file: %s ...exiting.\n",argv[fn+1]);
	    exit(1);
	  }
          /* determine whether this is the source or destination file */
          if (READ_OP(tsdata->tsh_tte[0].tte_op_type))  {
            if (src != NULL) {
              fprintf(stderr,"You passed 2 source dump files.\n");
              fprintf(stderr,"Please pass matching source and destination dumps or a single dump .\n");
              exit(1);
            }
            src = tsdata;
            srcfilename = argv[fn+1];
          }
          else 
          if (WRITE_OP(tsdata->tsh_tte[0].tte_op_type)) {
            if (dst != NULL) {
              fprintf(stderr,"You passed 2 destination dump files.\n");
              fprintf(stderr,"Please pass matching source and destination dumps or a single dump .\n");
              exit(1);
            }
            dst = tsdata;
            dstfilename = argv[fn+1];
          } 
          else {
             fprintf(stderr,"Timestamp dump had invalid operation in tte[0]: %s\n",argv[fn+1]);
             exit(1);
          }
          /* If these files are from the same transfer, they should be the same size. */
          if (src->tsh_tt_bytes != dst->tsh_tt_bytes) {
            fprintf(stderr,"Source and destination files are not the same size.\n");
            fprintf(stderr,"Please pass matching source and destination dumps.\n");
            exit(1);
          }
        }

        /* get total threads */
        if (src != NULL) xdd_getthreads(src,&total_threads_src, thread_id_src, &op_mix);
        if (dst != NULL) xdd_getthreads(dst,&total_threads_dst, thread_id_dst, &op_mix);
        /* If e2e, better have same number of threads. */
        if (total_threads_src > 0 && total_threads_dst > 0 && total_threads_src != total_threads_dst) {
          fprintf(stderr,"Warning: source (%d) and destination (%d) files don't have the same number of I/O threads.\n",
                total_threads_src, total_threads_dst);
        }

        /* match-add kernel events with xdd events if specfied */
        /* at this point, src & dst contain all xdd events     */
        /* note that iotrace_data_dir is independent of $PWD   */
        if (kernel_trace)
        {
          if ((iotrace_data_dir=getenv("TRACE_LOG_LOC")) == NULL)
               iotrace_data_dir=getenv("HOME");
               current_work_dir=getenv("PWD");
     
          if (src != NULL) {
            sprintf(kernfilename,"decode %s/dictionary* %s/iotrace_data.%d.out",
	       	iotrace_data_dir, iotrace_data_dir, src->tsh_target_thread_id);
            if ( system(kernfilename) == -1 ) {
               fprintf(stderr,"in main: shell command failed: %s\n",kernfilename);
               exit(1);
            }
            if (strcmp(iotrace_data_dir,current_work_dir) != 0) {
              sprintf(kernfilename,"mv %s/iotrace_data.%d.out.ascii %s",
	  	iotrace_data_dir, src->tsh_target_thread_id,getenv("PWD"));
              if ( system(kernfilename) == -1 ) {
                 fprintf(stderr,"in main: shell command failed: %s\n",kernfilename);
                 exit(1);
              }
            }
            sprintf(kernfilename,"%s/iotrace_data.%d.out.ascii",
                current_work_dir, src->tsh_target_thread_id);
            matchadd_kernel_events(1,total_threads_src,thread_id_src,kernfilename,src);
            if (op_mix > 0.0)
            matchadd_kernel_events(0,total_threads_src,thread_id_src,kernfilename,src);
            /* write out the src and dst data structs that now contain kernel data */
            sprintf(kernfilename,"%s/%sk", current_work_dir, srcfilename);
            retval = xdd_writefile(kernfilename,src,tsdata_size);
            if (retval == 0) {
		fprintf(stderr,"xdd_writefile() failed to write %s ...exiting.\n",kernfilename);
		exit(1);
	    }
          }

          if (dst != NULL) {
            sprintf(kernfilename,"decode %s/dictionary* %s/iotrace_data.%d.out",
		iotrace_data_dir, iotrace_data_dir, dst->tsh_target_thread_id);
            if ( system(kernfilename) == -1 ) {
               fprintf(stderr,"in main: shell command failed: %s\n",kernfilename);
               exit(1);
            }
            if (strcmp(iotrace_data_dir,current_work_dir) != 0) {
              sprintf(kernfilename,"mv %s/iotrace_data.%d.out.ascii %s",
		iotrace_data_dir, dst->tsh_target_thread_id,getenv("PWD"));
              if ( system(kernfilename) == -1 ) {
                 fprintf(stderr,"in main: shell command failed: %s\n",kernfilename);
                 exit(1);
              }
            }
            sprintf(kernfilename,"%s/iotrace_data.%d.out.ascii",
                current_work_dir, dst->tsh_target_thread_id);
            matchadd_kernel_events(0,total_threads_dst,thread_id_dst,kernfilename,dst);
            if (op_mix > 0.0)
            matchadd_kernel_events(1,total_threads_dst,thread_id_dst,kernfilename,dst);
            sprintf(kernfilename,"%s/%sk", current_work_dir, dstfilename);
            retval = xdd_writefile(kernfilename,dst,tsdata_size);
            if (retval == 0) {
		fprintf(stderr,"xdd_writefile() failed to write %s ...exiting.\n",kernfilename);
		exit(1);
	    }
          }

        }

	/* get the MIN timestamp normalize the times */
	if (src!=NULL) normalize_time(src,&src_start_norm);
	if (dst!=NULL) normalize_time(dst,&dst_start_norm);

	/* sort timestamp entries for each operation (read/send/recv/write) */
        /* kernel events are co-located with matching xdd op */
        if (src!=NULL) {
          sort_by_time(src, DISK_END, &read_op);
          if (dst!=NULL) {
          sort_by_time(src, NET_END,  &send_op);
          }
        }
        if (dst!=NULL) {
          sort_by_time(dst, DISK_END, &write_op);
          if (src!=NULL) {
          sort_by_time(dst, NET_END,  &recv_op);
          }
        }

	/* write the outfile(s) */
        write_outfile  (src,dst,read_op,send_op,recv_op,write_op);

	/* free memory */
	free(read_op);
	free(send_op);
	free(recv_op);
	free(write_op);
	free(src);
	free(dst);

	return 0;
}
/* get total threads, thread ids, operation mix */
void
xdd_getthreads(xdd_ts_header_t *tsdata, int *total_threads, int thread_id[], double *op_mix)
{
    size_t i;
       int k = -1, tothreads = 0;
       uint64_t read_ops = 0, write_ops = 0;
       if (*op_mix > 0.0) {
          fprintf(stderr,"xdd_getthreads: op_mix = %10.4f ..should be 0.0\n",*op_mix);
          exit (-1);
       }
        /* how many qthreads are there? */
        for (i = 0; i < tsdata->tsh_tt_size; i++) {
                tothreads = MAX(tsdata->tsh_tte[i].tte_worker_thread_number,tothreads);
                if (k < tothreads && k < MAX_WORKER_THREADS)
                {
                            k = tothreads;
                  thread_id[k] = tsdata->tsh_tte[i].tte_thread_id;
                }
                read_ops  +=  READ_OP(tsdata->tsh_tte[i].tte_op_type);
                write_ops += WRITE_OP(tsdata->tsh_tte[i].tte_op_type);
        }
        *total_threads = tothreads + 1;
        *op_mix        = (read_ops && write_ops) ? (double)read_ops/(double)write_ops : 0.0;
       return;
}

/* subtract the timestamps by the minimum start times */
void normalize_time(xdd_ts_header_t *tsdata, nclk_t *start_norm) {
	size_t i;
	nclk_t start;
	start = tsdata->tsh_tte[0].tte_disk_start;

	/* get minimum starting times */
	for (i=0; i < tsdata->tsh_tt_size; i++) {
		if (!READ_OP(tsdata->tsh_tte[i].tte_op_type) && !WRITE_OP(tsdata->tsh_tte[i].tte_op_type))
			continue;
		start = MIN(start, tsdata->tsh_tte[i].tte_disk_start);
		if (tsdata->tsh_tte[i].tte_net_start == 0)
			continue;
		start = MIN(start, tsdata->tsh_tte[i].tte_net_start);
	}

        *start_norm = start;

	/* normalize timestamps */
	for (i=0; i < tsdata->tsh_tt_size; i++) {
		/* don't subtract from zero */
		if (EOF_OP(tsdata->tsh_tte[i].tte_op_type) && (tsdata->tsh_tte[i].tte_disk_start == 0))
			continue;
		tsdata->tsh_tte[i].tte_disk_start   -= start;
		tsdata->tsh_tte[i].tte_disk_start_k -= start;
		tsdata->tsh_tte[i].tte_disk_end     -= start;
		tsdata->tsh_tte[i].tte_disk_end_k   -= start;
	}

	/* normalize timestamps */
	for (i=0; i < tsdata->tsh_tt_size; i++) {
		/* don't subtract from zero */
		if (EOF_OP(tsdata->tsh_tte[i].tte_op_type) && (tsdata->tsh_tte[i].tte_net_start == 0))
			continue;
		tsdata->tsh_tte[i].tte_net_start    -= start;
		tsdata->tsh_tte[i].tte_net_start_k  -= start;
		tsdata->tsh_tte[i].tte_net_end      -= start;
		tsdata->tsh_tte[i].tte_net_end_k    -= start;
	}
}


/* custom quick sort for the xdd_ts_tte_t array */
void tte_qsort(xdd_ts_tte_t **list, size_t key_offset, int64_t low, int64_t high) {
	/* NOTE: the pointer arithmetic is intended to take 'key_offset'
	 * and use it to access either disk_start, disk_end, net_start, or net_end
	 * in a xdd_ts_tte_t element.  So if you pass a list of (xdd_ts_tte_t *) elements,
	 * and you want to sort the pointers based on what is in list[i].tte_disk_start,
	 * pass offsetof(xdd_ts_tte_t,disk_start) as key_offset.  This function is ONLY
	 * intended to sort by nclk_t timestamps at the moment.  Passing any other
	 * offset will break stuff. */

	/* get the nclk_t time value (specified by key_offset)
	 * from the given xdd_ts_tte_t element in the list */
	#define timevalue(i) ( *((nclk_t *)((char *)list[i] + key_offset)) )

	int64_t i,j;
	xdd_ts_tte_t *tmp = NULL;
	nclk_t mid;

	i = low;
	j = high;
	/* comparison value */
	mid = timevalue((low+high)/2);

	/* partition */
	while (i <= j) {
		/* find element above and below */
		while (timevalue(i) < mid) i++;
		while (timevalue(j) > mid) j--;

		if (i <= j) {
			/* swap elements */
			tmp = list[i];
			list[i] = list[j];
			list[j] = tmp;
			i++;
			j--;
		}
	}

	/* recurse */
	if (low  < j) tte_qsort(list, key_offset, low,    j);
	if (high > i) tte_qsort(list, key_offset,   i, high);
}

/* sort timestamp entries by completion time */
void sort_by_time(xdd_ts_header_t *tsdata, size_t op_offset, xdd_ts_tte_t ***op_sorted )
		                                                         {
	/* allocate array of pointers to tte structs */
	xdd_ts_tte_t **op = malloc(tsdata->tsh_tt_size*sizeof(xdd_ts_tte_t *));
	if (op == NULL) {
		fprintf(stderr,"Could not allocate enough memory for quick sort.\n");
		exit(1);
	}

	/* initialize pointers */
	size_t i;
	for (i = 0; i < tsdata->tsh_tt_size; i++) {
		op[i] = &(tsdata->tsh_tte[i]);
	}

	/* sort pointers */
	tte_qsort(op, op_offset,0, tsdata->tsh_tt_size-1);

	/* save pointers */
	*op_sorted = op;
}
/* write the outfile */
void write_outfile(xdd_ts_header_t *src, xdd_ts_header_t *dst, xdd_ts_tte_t **read_op,
	xdd_ts_tte_t **send_op, xdd_ts_tte_t **recv_op, xdd_ts_tte_t **write_op) {

    int i;
	int64_t   k, numts_entries;
	float cutoff;
	/* variables for the file writing loop below */
	FILE *outfile;
	/* bandwidths for each window in time */
        double op_time[4], op_mbs[4];
        nclk_t  op_dks[4], op_dke[4];
        int32_t max_xfer;
        char line[OUTFILENAME_LEN] = {" "};
        char datfilename[OUTFILENAME_LEN] = {" "};

        /* create analysis directory, unless it's cwd */
        if (strcmp(outfilebase,".") != 0) {
		sprintf(line,"mkdir -p %s",outfilebase);
        	if ( system(line) == -1 ) {
		  fprintf(stderr,"shell command failed: %s\n",line);
		  exit(1);
        	}
        }
        sprintf(datfilename,"%s/analysis.dat",outfilebase);
	/* try to open file */
	if (strlen(datfilename)==0) {
		outfile = stdout;
	} else {
		outfile = fopen(datfilename, "w");
	}
	/* do we have a file pointer? */
	if (outfile == NULL) {
		fprintf(stderr,"Can not open output file: %s\n",datfilename);
		exit(1);
	}

	/* file header */
	numts_entries = 0;
        if (src != NULL) {
              numts_entries = src->tsh_tt_size;
              if (src != NULL)
                  fprintf(outfile,"#SOURCE");
              else
                  fprintf(outfile,"#READ OP");
	      fprintf(outfile," timestamp file: %s\n",srcfilename);
	      fprintf(outfile,"#timestamp: %s",src->tsh_td);
	      fprintf(outfile,"#reqsize: %d\n",src->tsh_reqsize);
	      fprintf(outfile,"#filesize: %ld\n",src->tsh_reqsize*src->tsh_tt_size);
              fprintf(outfile,"#qthreads_src, target pid, pids: %d %d ",src->tsh_target_thread_id,total_threads_src);
          for (i = 0; i < total_threads_src; i++) { 
              fprintf(outfile,"%d ",thread_id_src[i] );
              }
              fprintf(outfile,"\n");
              fprintf(outfile,"#src_start_norm %lld\n",src_start_norm);
        }
        if (dst != NULL) {
              numts_entries = dst->tsh_tt_size;
              if (src != NULL) fprintf(outfile,"#DESTINATION ");
              else             fprintf(outfile,"#WRITE OP ");
	      fprintf(outfile," timestamp file: %s\n",dstfilename);
	      fprintf(outfile,"#timestamp: %s",dst->tsh_td);
	      fprintf(outfile,"#reqsize: %d\n",dst->tsh_reqsize);
	      fprintf(outfile,"#filesize: %ld\n",dst->tsh_reqsize*dst->tsh_tt_size);
              fprintf(outfile,"#qthreads_dst, target pid, pids: %d %d ",dst->tsh_target_thread_id,total_threads_dst);
            for (i = 0; i < total_threads_dst; i++) { 
	      fprintf(outfile,"%d ",thread_id_dst[i] );
            }
              fprintf(outfile,"\n");
              fprintf(outfile,"#dst_start_norm %lld\n",dst_start_norm);
        }
        fprintf(outfile,"#   read_time     read_bw read_start   read_end   send_time      read_bw send_start   send_end");
        fprintf(outfile,"#   recv_time     read_bw recv_start   recv_end  write_time     write_bw write_start  write_end\n");
        fprintf(outfile,"#    (s)           (MB/s)    (ns)         (ns)      (s)          (MB/s)      (ns)        (ns) ");
        fprintf(outfile,"#                            (ns)         (ns)                               (ns)        (ns)\n");
                
	/* loop through tte entries */
	for (i = 0; i < numts_entries; i++) {
		/* make sure this is an op that matters */
                max_xfer = 0;
                for (k=0; k < 4; k++) {
                  op_time[k] = 0.0;
                  op_mbs [k] = 0.0;
                  op_dks [k] = 0;
                  op_dke [k] = 0;
                }

                if (read_op != NULL) {
		  /* read disk */
                 if (read_op[i]->tte_disk_end > 0) {
                  cutoff = MAX( nclk2sec(read_op[i]->tte_disk_end)-window_size, 0.0);
                  for (k = i; (k >= 0) && ( nclk2sec(read_op[k]->tte_disk_end) > cutoff); k--)
			op_mbs [0] += (double)read_op[k]->tte_disk_xfer_size;
			op_mbs [0]  = op_mbs[0] / MIN((double)window_size,(double)nclk2sec(read_op[i]->tte_disk_end)) / BPU;
			op_time[0]  = nclk2sec(read_op[i]->tte_disk_end);
			max_xfer = MAX(max_xfer,read_op[i]->tte_disk_xfer_size);
                  if (kernel_trace) {
			op_dks [0]  = read_op[i]->tte_disk_start_k  - read_op[i]->tte_disk_start;
			op_dke [0]  = read_op[i]->tte_disk_end      - read_op[i]->tte_disk_end_k;
                  }
                 }
                }
                if (send_op != NULL) {
		/* send net */
                 if (send_op[i]->tte_net_end > 0) {
                  cutoff = MAX( nclk2sec(send_op[i]->tte_net_end)-window_size ,0.0);
                  for (k = i; (k >= 0) && ( nclk2sec(send_op[k]->tte_net_end) > cutoff); k--)
			op_mbs [1] += (double)send_op[k]->tte_net_xfer_size;
			op_mbs [1]  = op_mbs[1] / MIN((double)window_size,(double)nclk2sec(send_op[i]->tte_net_end)) / BPU;
			op_time[1]  = nclk2sec(send_op[i]->tte_net_end);
			max_xfer = MAX(max_xfer,send_op[i]->tte_net_xfer_size);
                  if (kernel_trace) {
			op_dks [1]  = send_op[i]->tte_net_start_k   - send_op[i]->tte_net_start;
			op_dke [1]  = send_op[i]->tte_net_end       - send_op[i]->tte_net_end_k;
                  }
                 }
                }
                if (recv_op != NULL) {
		/* recv net */
                 if (recv_op[i]->tte_net_end > 0) {
                  cutoff = MAX( nclk2sec(recv_op[i]->tte_net_end)-window_size, 0.0);
                  for (k = i; (k >= 0) && ( nclk2sec(recv_op[k]->tte_net_end) > cutoff); k--)
			op_mbs [2] += (double)recv_op[k]->tte_net_xfer_size;
			op_mbs [2]  = op_mbs[2] / MIN((double)window_size,(double)nclk2sec(recv_op[i]->tte_net_end)) / BPU;
			op_time[2]  = nclk2sec(recv_op[i]->tte_net_end);
			max_xfer    = MAX(max_xfer,recv_op[i]->tte_net_xfer_size);
                  if (kernel_trace) {
			op_dks [2]  = send_op[i]->tte_net_start_k   - send_op[i]->tte_net_start;
			op_dke [2]  = send_op[i]->tte_net_end       - send_op[i]->tte_net_end_k;
                  }
                 }
                }
                if (write_op != NULL) {
		/* write disk */
                 if (write_op[i]->tte_disk_end > 0) {
                  cutoff = MAX( nclk2sec(write_op[i]->tte_disk_end)-window_size, 0.0);
                  for (k = i; (k >= 0) && (nclk2sec(write_op[k]->tte_disk_end) > cutoff); k--)
			op_mbs [3] += (double)write_op[k]->tte_disk_xfer_size;
			op_mbs [3]  = op_mbs[3] / MIN((double)window_size,(double)nclk2sec(write_op[i]->tte_disk_end)) / BPU;
			op_time[3]  = nclk2sec(write_op[i]->tte_disk_end);
			max_xfer    = MAX(max_xfer,write_op[i]->tte_disk_xfer_size);
                  if (kernel_trace) {
			op_dks [3]  = write_op[i]->tte_disk_start_k - write_op[i]->tte_disk_start;
			op_dke [3]  = write_op[i]->tte_disk_end     - write_op[i]->tte_disk_end_k;
                  }
                 }
                }
                if (max_xfer == 0)
                   continue;
                memset(line,0,strlen(line));
		/* write to file. take care to leave no embedded '0'(nulls) between fields */
		for (k = 0; k < 4; k++) { 
                   sprintf(&line[k*45],"%12.6f %10.4f %10lld %10lld ",op_time[k],op_mbs[k],op_dks[k],op_dke[k]);
                   if ( op_time[k] == 0.0 ) strncpy(&line[k*45+3],"?",1);
                }
                fprintf(outfile,"%s\n",line);
	}
	fclose(outfile);
  /* write the gnuplot file */
        memset(line,0,strlen(line));
        if (strcmp(outfilebase,".") != 0) sprintf(line,"gnuplot_%s",basename(outfilebase));
	else				  sprintf(line,"gnuplot_");
        outfile = fopen(line, "w");
        /* do we have a file pointer? */
        if (outfile == NULL) { 
                fprintf(stderr,"Can not open gnuplot file for writing: %s\n",line);
                exit(1);
        }               
        fprintf(outfile,"set term po eps color\n");
        fprintf(outfile,"set pointsize 1\n");
        fprintf(outfile,"set ylabel \"MB/s\"\n");
        fprintf(outfile,"set xlabel \"Time in seconds\"\n");
        fprintf(outfile,"set style data line\n");

  if (read_op!=NULL && send_op!=NULL && recv_op!=NULL && write_op!=NULL) {
        fprintf(outfile,"set title \"XDDCP Transfer\"\n");
        fprintf(outfile,"set output \"%s/all.eps\"\n",                       outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  1:2  title 'read_disk'  lw 2,\\\n",     datfilename);
        fprintf(outfile,"'%s' using  5:6  title 'send_netw'  lw 2,\\\n",     datfilename);
        fprintf(outfile,"'%s' using  9:10 title 'recv_netw'  lw 2,\\\n",     datfilename);
        fprintf(outfile,"'%s' using 13:14 title 'write_disk' lw 2\n",        datfilename);
        fprintf(outfile,"set output \"%s/netw.eps\"\n",                      outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  5:6  title 'send_netw'  lw 2,\\\n",     datfilename);
        fprintf(outfile,"'%s' using  9:10 title 'recv_netw'  lw 2\n",        datfilename);
        fprintf(outfile,"set output \"%s/disk.eps\"\n",                      outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  1:2  title 'read_disk'  lw 2,\\\n",     datfilename);
        fprintf(outfile,"'%s' using 13:14 title 'write_disk' lw 2\n",        datfilename);
        fprintf(outfile,"set output \"%s/srce.eps\"\n",                      outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  1:2  title 'read_disk'  lw 2,\\\n",     datfilename);
        fprintf(outfile,"'%s' using  5:6  title 'send_netw'  lw 2\n",        datfilename);
        fprintf(outfile,"set output \"%s/dest.eps\"\n",                      outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  9:10 title 'recv_netw'  lw 2,\\\n",     datfilename);
        fprintf(outfile,"'%s' using 13:14 title 'write_disk' lw 2\n",        datfilename);
        fprintf(outfile,"set output \"%s/read.eps\"\n",                      outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  1:2  title 'read_disk'  lw 2\n",        datfilename);
        fprintf(outfile,"set output \"%s/send.eps\"\n",                      outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  5:6  title 'send_netw'  lw 2\n",        datfilename);
        fprintf(outfile,"set output \"%s/recv.eps\"\n",                      outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  9:10 title 'recv_netw'  lw 2\n",        datfilename);
        fprintf(outfile,"set output \"%s/write.eps\"\n",                     outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using 13:14 title 'write_disk' lw 2\n",        datfilename);
     if (kernel_trace) {
        fprintf(outfile,"set logscale y\n");
        fprintf(outfile,"set ylabel \"Delta Time (ns) |xdd-kernel|\"\n");
        fprintf(outfile,"set output \"%s/all_k.eps\"\n",                     outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  1:3  title 'read_disk_entr'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  1:4  title 'read_disk_exit'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  5:7  title 'send_netw_entr'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  5:8  title 'send_netw_exit'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  9:11 title 'recv_netw_entr'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  9:12 title 'recv_netw_exit'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using 13:15 title 'write_disk_entr' lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using 13:16 title 'write_disk_exit' lw 2\n",   datfilename);
        fprintf(outfile,"set output \"%s/netw_k.eps\"\n",                    outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  5:7  title 'send_netw_entr'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  5:8  title 'send_netw_exit'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  9:10 title 'recv_netw_entr'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  9:12 title 'recv_netw_exit'  lw 2\n",   datfilename);
        fprintf(outfile,"set output \"%s/disk_k.eps\"\n",                    outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  1:3  title 'read_disk_entr'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  1:4  title 'read_disk_exit'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using 13:15 title 'write_disk_entr' lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using 13:16 title 'write_disk_exit' lw 2\n",   datfilename);
        fprintf(outfile,"set output \"%s/srce_k.eps\"\n",                    outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  1:3  title 'read_disk_entr'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  1:4  title 'read_disk_exit'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  5:7  title 'send_netw_entr'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  5:8  title 'send_netw_exit'  lw 2\n",   datfilename);
        fprintf(outfile,"set output \"%s/dest_k.eps\"\n",                    outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  9:11 title 'recv_netw_entr'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  9:12 title 'recv_netw_exit'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using 13:15 title 'write_disk_entr' lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using 13:16 title 'write_disk_exit' lw 2\n",   datfilename);
        fprintf(outfile,"set output \"%s/read_k.eps\"\n",                    outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  1:3  title 'read_disk_entr'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  1:4  title 'read_disk_exit'  lw 2\n",   datfilename);
        fprintf(outfile,"set output \"%s/send_k.eps\"\n",                    outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  5:7  title 'send_netw_entr'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  5:8  title 'send_netw_exit'  lw 2\n",   datfilename);
        fprintf(outfile,"set output \"%s/recv_k.eps\"\n",                    outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  9:11 title 'recv_netw_entr'  lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using  9:12 title 'recv_netw_exit'  lw 2\n",   datfilename);
        fprintf(outfile,"set output \"%s/write_k.eps\"\n",                   outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using 13:15 title 'write_disk_entr' lw 2,\\\n",datfilename);
        fprintf(outfile,"'%s' using 13:16 title 'write_disk_exit' lw 2\n",   datfilename);
     }
  }
  else
  if (read_op!=NULL && op_mix == 0.0) {
        fprintf(outfile,"set title \"XDD Read\"\n");
        fprintf(outfile,"set output \"%s/read.eps\"\n",                       outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  1:2  title 'read_disk'  lw 2\n",         datfilename);
     if (kernel_trace) {
        fprintf(outfile,"set logscale y\n");
        fprintf(outfile,"set ylabel \"Delta Time (ns) |xdd-kernel|\"\n");
        fprintf(outfile,"set output \"%s/read_k.eps\"\n",                     outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  1:3  title 'read_disk_entr'  lw 2,\\\n", datfilename);
        fprintf(outfile,"'%s' using  1:4  title 'read_disk_exit'  lw 2\n",    datfilename);
     }
  }
  else
  if (write_op!=NULL && op_mix == 0.0) {
        fprintf(outfile,"set title \"XDD Write\"\n");
        fprintf(outfile,"set output \"%s/write.eps\"\n",                      outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using 13:14 title 'write_disk' lw 2\n",         datfilename);
     if (kernel_trace) {
        fprintf(outfile,"set logscale y\n");
        fprintf(outfile,"set ylabel \"Delta Time (ns) |xdd-kernel|\"\n");
        fprintf(outfile,"set output \"%s/write_k.eps\"\n",                    outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using 13:15 title 'write_disk_entr' lw 2,\\\n", datfilename);
        fprintf(outfile,"'%s' using 13:16 title 'write_disk_exit' lw 2\n",    datfilename);
     }
  }
  else 
  if (op_mix > 0.0) {
        fprintf(outfile,"set title \"XDD Mixed Read/Write %-7.0f%%\"\n",op_mix*100.0);
        fprintf(outfile,"set output \"%s/mixed.eps\"\n",                       outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  1:2  title 'mixed_disk'  lw 2\n",         datfilename);
     if (kernel_trace) {
        fprintf(outfile,"set logscale y\n");
        fprintf(outfile,"set ylabel \"Delta Time (ns) |xdd-kernel|\"\n");
        fprintf(outfile,"set output \"%s/mixed_k.eps\"\n",                     outfilebase);
        fprintf(outfile,"plot\\\n");
        fprintf(outfile,"'%s' using  1:3  title 'mixed_disk_entr'  lw 2,\\\n", datfilename);
        fprintf(outfile,"'%s' using  1:4  title 'mixed_disk_exit'  lw 2\n",    datfilename);
     }
  }
  else {
        fprintf(stderr,"write_outfile: Case not recognized\n");
        exit(1);
  }
	fclose(outfile);
        memset(line,0,strlen(line));
        if (strcmp(outfilebase,".") != 0)sprintf(line,"gnuplot gnuplot_%s",basename(outfilebase));
        else				 sprintf(line,"gnuplot gnuplot_  ");
        if ( system(line) == -1 ) { 
              fprintf(stderr,"shell command failed: %s\n",line);
              exit(1);
        }
        fprintf(stderr,"Check directory %s for analysis results\n",outfilebase);
}


/*********************************************************
 * Read an XDD binary timestamp dump into a structure
 *
 * IN:
 *   filename    - name of the .bin file to read
 * OUT:
 *   tsdata      - pointer to the timestamp structure
 *   tsdata_size - size malloc'd for the structure
 * RETURN:
 *   1 if succeeded, 0 if failed
 *********************************************************/
int xdd_readfile(char *filename, xdd_ts_header_t **tsdata, size_t *tsdata_size) {

	FILE *tsfd;
	size_t tsize = 0;
	xdd_ts_header_t *tdata = NULL;
	size_t result = 0;
	uint32_t magic = 0;

	/* open file */
	tsfd = fopen(filename, "rb");

	if (tsfd == NULL) {
		fprintf(stderr,"Can not open file: %s\n",filename);
		return 0;
	}

	/* get file length */
	fseek(tsfd,0,SEEK_END);
	tsize = ftell(tsfd);
	fseek(tsfd,0,SEEK_SET);

	if (tsize == 0) {
		fprintf(stderr,"File is empty: %s\n",filename);
		return 0;
	}

	/* allocate memory */
	tdata = malloc(tsize);

	if (tdata == NULL) {
		fprintf(stderr,"Could not allocate memory for file: %s\n",filename);
		return 0;
	}

	/* check magic number in xdd_ts_header_t */
	result = fread(&magic,sizeof(uint32_t),1,tsfd);
	fseek(tsfd,0,SEEK_SET);

	if (result == 0) {
		fprintf(stderr,"Error reading file: %s\n",filename);
		return 0;
	}
	if (magic != BIN_MAGIC_NUMBER) {
		fprintf(stderr,"File is not in a readable format: %s\n",filename);
		return 0;
	}

	/* read rest of structure */
	result = fread(tdata,tsize,1,tsfd);

	if (result == 0) {
		fprintf(stderr,"Error reading file: %s\n",filename);
		return 0;
	}

	/* no empty sets */
	if (tdata->tsh_tt_size < 1) {
		fprintf(stderr,"Timestamp dump was empty: %s\n",filename);
		return 0;
	}

	*tsdata = tdata;
	*tsdata_size = tsize;

	/* close file */
	fclose(tsfd);

	return 1;
}


/*********************************************************
 * Write an XDD binary timestamp dump from a structure
 *
 * IN:
 *   filename    - name of .bin file to write to
 *   tsdata      - pointer to timestamp structure
 *   tsdata_size - size of    timestamp structure
 * OUT:
 * RETURN:
 *   1 if succeeded, 0 if failed
 *********************************************************/
int xdd_writefile(char *filename, xdd_ts_header_t *tsdata, size_t tsdata_size) {

	FILE *tsfd;
	size_t result = 0;

	/* open file */
	tsfd = fopen(filename, "wb");

	if (tsfd == NULL) {
		fprintf(stderr,"Can not open file: %s\n",filename);
		return 0;
	}

	/* write structure */
	result = fwrite(tsdata,tsdata_size,1,tsfd);

	if (result == 0) {
		fprintf(stderr,"Error writing file: %s\n",filename);
		return 0;
	}

	/* close file */
	fclose(tsfd);

	return 1;
}


/********************************************
 * getoptions()
 *
 * Parses argument list for options
 *
 * Any arguments in argv that are not
 * recognized here are assumed to be files.
 * This function returns the number of
 * the first non-option argument.  It is up
 * to xdd_readfile() to make sure they
 * are actual filenames.
 ********************************************/
int getoptions(int argc, char **argv) {

	int ierr, opt, argnum;
	extern char *optarg;
	ierr = 0;
	argnum = 1; /* track number of args parsed */

	/* set default options */
	strcpy(outfilebase,".");
	window_size = 1;

	/* loop through options */
	while ((opt = getopt(argc, argv, "t:ko:h")) != -1) {
		switch (opt) {
			case 't': /* moving average */
				window_size = atoi(optarg);
				argnum += 2;
				if (window_size <= 0) {
					fprintf(stderr,"\nwindow size must be more than 0.\n\n");
					ierr++;
				}
				break;
                        case 'k': /* use src, dst kernel trace files */
                                kernel_trace = 1;
                                argnum += 1;
                                break;
			case 'o': /* output file name */
				strncpy(outfilebase,optarg,OUTFILENAME_LEN);
				outfilebase[OUTFILENAME_LEN-1] = '\0';
				argnum += 2;
				if (strlen(outfilebase) == 0)
					ierr++;
				break;
			case 'h': /* help */
			default:
				printusage(argv[0]);
				break;
		}
	}

	/* do we have two filenames and no parsing errors? */
	if ( (argc-argnum < 1) || (ierr != 0) ) {
		printusage(argv[0]);
	}

	/* return the number of the first non-option argument */
	return argnum;
}


/* prints some help text and exits */
void printusage(char *progname) {
	fprintf(stderr, "\n");
	fprintf(stderr, "USAGE: %s [OPTIONS] TimestampFile [TimestampFile]\n\n",progname);

	fprintf(stderr, "This program takes 1 or 2 XDD timestamp dump files\n");
	fprintf(stderr, "from single node I/O operations or 2 node file transfer,\n");
        fprintf(stderr, "and determines the approximate bandwidth at\n");
	fprintf(stderr, "any given time by using a sliding window approach.\n");
	fprintf(stderr, "If 1 file  is  given, read, write, or mixed operations on 1 node are assumed\n");
	fprintf(stderr, "If 2 files are given, an e2e file transfer between 2 nodes is assumed\n\n");

	fprintf(stderr, "For instance, using '-t 5' will set the window size to 5 seconds\n");
	fprintf(stderr, "At each point in time during the transfer, it will calculate\n");
	fprintf(stderr, "the amount of data committed during the past 5 seconds.\n");
	fprintf(stderr, "The amount of data committed during the first 5 seconds is prorated.\n");
	fprintf(stderr, "It outputs the bandwidth during that window, so you can plot the\n");
	fprintf(stderr, "bandwidth as a function of running time for each operation.\n");
	fprintf(stderr, "In the case of e2e file transfers, operations consist of:\n");
	fprintf(stderr, "   read (src) -> send (src) -> recv (dst) -> write (dst)\n\n");

	fprintf(stderr, "Analysis results are located in directory [directory]. \n");
	fprintf(stderr, "Results are included in file 'analysis.dat' and plots in '*.eps' files\n");
	fprintf(stderr, "Edit file 'gnuplot_[directory]' as desired and run 'gnuplot gnuplot_[directory]'\n");
        fprintf(stderr, "to regenerate\n\n");

	fprintf(stderr, "To generate and analyze a timestamp dump from XDD, use the following procedure:\n");
	fprintf(stderr, "*Assumes iotrace kernel module installed and utilities 'iotrace_init' and 'decode' in $PATH*\n\n");
	fprintf(stderr, "     1. specify iotrace log files location (default $HOME) with: \n\n");
	fprintf(stderr, "           'export TRACE_LOG_LOC=$PWD' \n\n");
	fprintf(stderr, "     2. run XDD using these examples as a guide:\n\n");
	fprintf(stderr, "         '             ${full path}/xdd -op [read | write] -ts dump [filename] [other xdd options..]'\n");
	fprintf(stderr, "        ( This generates XDD timestamp dump file: filename].bin\n\n");
	fprintf(stderr, "         'iotrace_init ${full path}/xdd -op [read | write] -ts dump [filename] [other xdd options..]'\n");
	fprintf(stderr, "        ( This generates both XDD and kernel timestamp dump files:\n\n");
	fprintf(stderr, "         [filename].bin, iotrace_data.PID.out, dictionary.DATE )\n\n");
	fprintf(stderr, "     3. execute %s using the following example as a guide:\n\n",progname);
	fprintf(stderr, "           '%s -t [secs] -k -o [result_dir] [filename].bin'\n\n",progname);
	fprintf(stderr, "        ( [secs] moving average, plot results in directory [results_dir]. \n\n");
	fprintf(stderr, "          if '-k' specified, %s generates files:\n",progname);
        fprintf(stderr, "             'iotrace_data.PID.out.ascii' ascii version of iotrace_data.PID.out\n\n");
        fprintf(stderr, "              '[filename].bink' combined timestamp file with XDD+kernel data)\n\n");
	fprintf(stderr, "In XDDCP, the -w or -W flag invokes this program and analysis automatically.\n\n");

	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "\t -h             \t print this usage text\n");
	fprintf(stderr, "\t -o [directory] \t analysis results directory w analysis.dat & *.eps files (default 'PWD') \n");
	fprintf(stderr, "\t -t <seconds>   \t number of seconds in the sliding window (default: %d)\n", window_size);
	fprintf(stderr, "\t -k             \t also analyze kernel trace data\n");

	fprintf(stderr, "\n");
	exit(1);
}


