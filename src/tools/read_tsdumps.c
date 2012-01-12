
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <xdd.h>

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
#define READ_OP(x)  (x==OP_TYPE_READ||x==SO_OP_READ)
#define WRITE_OP(x) (x==OP_TYPE_WRITE||x==SO_OP_WRITE||x==SO_OP_WRITE_VERIFY)
#define NO_OP(x)    (x==OP_TYPE_NOOP||x==SO_OP_NOOP)
#define EOF_OP(x)   (x==OP_TYPE_EOF||x==SO_OP_EOF)

/* key offsets in tte_t for the sorting function */
#define DISK_START  offsetof(tte_t, disk_start)
#define DISK_END    offsetof(tte_t, disk_end)
#define NET_START   offsetof(tte_t, net_start)
#define NET_END     offsetof(tte_t, net_end)

/* magic number to make sure we can read the file */
#define BIN_MAGIC_NUMBER 0xDEADBEEF

#define MAX_QTHREADS 1024
int thread_id_src[MAX_QTHREADS];
int thread_id_dst[MAX_QTHREADS];
int total_threads_src;
int total_threads_dst;
        nclk_t src_start_norm,dst_start_norm;

/* output file, if specified */
#define OUTFILENAME_LEN 512
char outfilename[OUTFILENAME_LEN];
char kernfilename[OUTFILENAME_LEN];

int kernel_trace = 0;
/* number of seconds in the sliding window */
int window_size;

/* normalize timestamps */
void normalize_time(tthdr_t *src, tthdr_t *dst);
/* custom quick sort for the tte_t array */
void tte_qsort(tte_t **list, size_t key_offset, int64_t left, int64_t right);
/* sorts the timestamp entries by completion time */
void sort_by_time(tthdr_t *src, tthdr_t *dst, tte_t ***read_op,
		tte_t ***send_op, tte_t ***recv_op, tte_t ***write_op);
/* write all the outfiles */
void write_outfile(tthdr_t *src, tthdr_t *dst, tte_t **read_op,
		tte_t **send_op, tte_t **recv_op, tte_t **write_op);
void write_outfile_k(tthdr_t *src, tthdr_t *dst, tte_t **read_op,
		tte_t **send_op, tte_t **recv_op, tte_t **write_op);
void write_outfile_d(tthdr_t *src, tthdr_t *dst, tte_t **read_op,
                tte_t **send_op, tte_t **recv_op, tte_t **write_op);
/* read, check, and store the src and dst file data */
int xdd_getdata(char *file1, char *file2, tthdr_t **src, tthdr_t **dst, size_t *tsdata_size);
/* Read an XDD binary timestamp dump into a structure */
int xdd_readfile(char *filename, tthdr_t **tsdata, size_t *tsdata_size);
/* Write an XDD binary timestamp structure to file */
int xdd_writefile(char *filename, tthdr_t *tsdata, size_t tsdata_size);
/* parse command line options */
int getoptions(int argc, char **argv);
/* print command line usage */
void printusage(char *progname);
/* get total threads, thread pids */
void xdd_getthreads(tthdr_t *src, tthdr_t *dst);

int
matchadd_kernel_events(int issource, int nthreads, int thread_id[], char *filespec, tthdr_t *xdd_data);


int main(int argc, char **argv) {

	int fn,argnum,retval;
        size_t tsdata_size;
        char *iotrace_data_dir;
	/* tsdumps for the source and destination sides */
	tthdr_t *src = NULL;
	tthdr_t *dst = NULL;
	/* timestamp entries sorted by ending time */
	tte_t **read_op = NULL;
	tte_t **send_op = NULL;
	tte_t **recv_op = NULL;
	tte_t **write_op = NULL;

	/* get command line options */
	argnum = getoptions(argc, argv);
	fn = MAX(argnum,1);

	/* get the src and dst data structs */
	retval = xdd_getdata(argv[fn],argv[fn+1],&src,&dst,&tsdata_size);
	if (retval == 0) {
		fprintf(stderr,"xdd_getdata() failed... exiting.\n");
		exit(1);
	}
        /* get total threads */
        xdd_getthreads(src,dst);

        /* match-add kernel events with xdd events if specfied */
        /* at this point, src & dst contain all xdd events     */
        if (kernel_trace)
        {
          if ((iotrace_data_dir=getenv("TR_RUN_LOG")) == NULL)
               iotrace_data_dir=getenv("HOME");
     
          sprintf(kernfilename,"decode %s/dictionary* %s/iotrace_data.%d.out",
		iotrace_data_dir, iotrace_data_dir, src->target_thread_id);
          system(kernfilename);
		fprintf(stderr,"kernfilename %s tt_size %lld\n",kernfilename,src->tt_size);
          sprintf(kernfilename,"mv %s/iotrace_data.%d.out.ascii %s",
		iotrace_data_dir, src->target_thread_id,getenv("PWD"));
          system(kernfilename);
		fprintf(stderr,"kernfilename %s\n",kernfilename);
          sprintf(kernfilename,"%s/iotrace_data.%d.out.ascii",
                getenv("PWD"),src->target_thread_id);
		fprintf(stderr,"kernfilename %s\n",kernfilename);
          matchadd_kernel_events(1,total_threads_src,thread_id_src,kernfilename,src);

          sprintf(kernfilename,"decode %s/dictionary* %s/iotrace_data.%d.out",
		iotrace_data_dir, iotrace_data_dir, dst->target_thread_id);
          system(kernfilename);
		fprintf(stderr,"kernfilename %s tt_size %lld\n",kernfilename,dst->tt_size);
          sprintf(kernfilename,"mv %s/iotrace_data.%d.out.ascii %s",
		iotrace_data_dir, dst->target_thread_id,getenv("PWD"));
          system(kernfilename);
		fprintf(stderr,"kernfilename %s\n",kernfilename);
          sprintf(kernfilename,"%s/iotrace_data.%d.out.ascii",
                getenv("PWD"),dst->target_thread_id);
		fprintf(stderr,"kernfilename %s\n",kernfilename);
          matchadd_kernel_events(0,total_threads_dst,thread_id_dst,kernfilename,dst);
          /* write out the src and dst data structs that now contain kernel data */
          sprintf(kernfilename,"%s/%sk", getenv("PWD"),argv[fn]);
		fprintf(stderr,"kernfilename %s\n",kernfilename);
          retval = xdd_writefile(kernfilename,src,tsdata_size);
          if (retval == 0) {
		fprintf(stderr,"xdd_writefile() failed... exiting.\n");
		exit(1);
	  }
          sprintf(kernfilename,"%s/%sk", getenv("PWD"),argv[fn+1]);
		fprintf(stderr,"kernfilename %s\n",kernfilename);
          retval = xdd_writefile(kernfilename,dst,tsdata_size);
          if (retval == 0) {
		fprintf(stderr,"xdd_writefile() failed... exiting.\n");
		exit(1);
	  }
        }

	/* get the MIN timestamp on each side and normalize the times */
	normalize_time(src,dst);

	/* sort timestamp entries for each operation (read/send/recv/write) */
        /* kernel events co-located with matching xdd op */
	sort_by_time(src,dst,&read_op,&send_op,&recv_op,&write_op);

	/* write the outfile(s) */
                          write_outfile  (src,dst,read_op,send_op,recv_op,write_op);
        if (kernel_trace) write_outfile_k(src,dst,read_op,send_op,recv_op,write_op);
        if (kernel_trace) write_outfile_d(src,dst,read_op,send_op,recv_op,write_op);

	/* free memory */
	free(read_op);
	free(send_op);
	free(recv_op);
	free(write_op);
	free(src);
	free(dst);

	return 0;
}

void
xdd_getthreads(tthdr_t *src, tthdr_t *dst)
{       
       int i,k;
        /* how many qthreads are there? */
        total_threads_src = 0;
        k = -1;
        for (i = 0; i < src->tt_size; i++) {
                total_threads_src = MAX(src->tte[i].qthread_number,total_threads_src);
                if (k < total_threads_src && k < MAX_QTHREADS)
                {
                                k = total_threads_src;
                  thread_id_src[k] = src->tte[i].thread_id;
                }
        }
        total_threads_src += 1;
        total_threads_dst = 0;
        k = -1;
        for (i = 0; i < dst->tt_size; i++) {
                total_threads_dst = MAX(dst->tte[i].qthread_number,total_threads_dst);
                if (k < total_threads_dst && k < MAX_QTHREADS)
                {
                                k = total_threads_dst;
                  thread_id_dst[k] = dst->tte[i].thread_id;
                }
        }
        total_threads_dst += 1;
}

/* subtract the timestamps by the minimum start times */
void normalize_time(tthdr_t *src, tthdr_t *dst) {
	int64_t i;
	nclk_t src_start,dst_start;
	src_start = src->tte[0].disk_start;
	dst_start = dst->tte[0].disk_start;

	/* get minimum starting times */
	for (i=0; i < src->tt_size; i++) {
		if (!READ_OP(src->tte[i].op_type) && !WRITE_OP(src->tte[i].op_type))
			continue;
		src_start = MIN(src_start, src->tte[i].disk_start);
		src_start = MIN(src_start, src->tte[i].net_start);
	}
	for (i=0; i < dst->tt_size; i++) {
		if (!READ_OP(dst->tte[i].op_type) && !WRITE_OP(dst->tte[i].op_type))
			continue;
		dst_start = MIN(dst_start, dst->tte[i].disk_start);
		dst_start = MIN(dst_start, dst->tte[i].net_start);
	}

        src_start_norm = src_start;
        dst_start_norm = dst_start;

	/* normalize timestamps */
	for (i=0; i < src->tt_size; i++) {
		src->tte[i].net_start    -= src_start;
		src->tte[i].net_start_k  -= src_start;
		src->tte[i].net_end      -= src_start;
		src->tte[i].net_end_k    -= src_start;
		/* don't subtract from zero */
		if (EOF_OP(src->tte[i].op_type) && (src->tte[i].disk_start == 0))
			continue;
		src->tte[i].disk_start   -= src_start;
		src->tte[i].disk_start_k -= src_start;
		src->tte[i].disk_end     -= src_start;
		src->tte[i].disk_end_k   -= src_start;
	}
	for (i=0; i < dst->tt_size; i++) {
		dst->tte[i].net_start    -= dst_start;
		dst->tte[i].net_start_k  -= dst_start;
		dst->tte[i].net_end      -= dst_start;
		dst->tte[i].net_end_k    -= dst_start;
		/* don't subtract from zero */
		if (EOF_OP(dst->tte[i].op_type) && (dst->tte[i].disk_start == 0))
			continue;
		dst->tte[i].disk_start   -= dst_start;
		dst->tte[i].disk_start_k -= dst_start;
		dst->tte[i].disk_end     -= dst_start;
		dst->tte[i].disk_end_k   -= dst_start;
	}
}


/* custom quick sort for the tte_t array */
void tte_qsort(tte_t **list, size_t key_offset, int64_t low, int64_t high) {
	/* NOTE: the pointer arithmetic is intended to take 'key_offset'
	 * and use it to access either disk_start, disk_end, net_start, or net_end
	 * in a tte_t element.  So if you pass a list of (tte_t *) elements,
	 * and you want to sort the pointers based on what is in list[i]->disk_start,
	 * pass offsetof(tte_t,disk_start) as key_offset.  This function is ONLY
	 * intended to sort by nclk_t timestamps at the moment.  Passing any other
	 * offset will break stuff. */

	/* get the nclk_t time value (specified by key_offset)
	 * from the given tte_t element in the list */
	#define timevalue(i) ( *((nclk_t *)((char *)list[i] + key_offset)) )

	int64_t i,j;
	tte_t *tmp = NULL;
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
	if (low < j)
		tte_qsort(list, key_offset, low, j);
	if (high > i)
		tte_qsort(list, key_offset, i, high);
}


/* sort timestamp entries by completion time */
void sort_by_time(tthdr_t *src, tthdr_t *dst, tte_t ***read_op,
		tte_t ***send_op, tte_t ***recv_op, tte_t ***write_op) {

	/* allocate array of pointers to tte structs */
	tte_t **reado = malloc(src->tt_size*sizeof(tte_t *));
	tte_t **sendo = malloc(src->tt_size*sizeof(tte_t *));
	tte_t **recvo = malloc(dst->tt_size*sizeof(tte_t *));
	tte_t **writeo = malloc(dst->tt_size*sizeof(tte_t *));
	if (reado == NULL || sendo == NULL || recvo == NULL || writeo == NULL) {
		fprintf(stderr,"Could not allocate enough memory for quick sort.\n");
		exit(1);
	}

	/* initialize pointers */
	int64_t i;
	for (i = 0; i < src->tt_size; i++) {
		reado[i] = &(src->tte[i]);
		sendo[i] = &(src->tte[i]);
		recvo[i] = &(dst->tte[i]);
		writeo[i]= &(dst->tte[i]);
	}

	/* sort pointers */
	tte_qsort(reado, DISK_END,0, src->tt_size-1);
	tte_qsort(sendo, NET_END, 0, src->tt_size-1);
	tte_qsort(recvo, NET_END, 0, dst->tt_size-1);
	tte_qsort(writeo,DISK_END,0, dst->tt_size-1);

	/* save pointers */
	*read_op = reado;
	*send_op = sendo;
	*recv_op = recvo;
	*write_op = writeo;
}

/* write the outfile */
void write_outfile(tthdr_t *src, tthdr_t *dst, tte_t **read_op,
		tte_t **send_op, tte_t **recv_op, tte_t **write_op) {

	int64_t i,k;
	float cutoff;
	/* variables for the file writing loop below */
	FILE *outfile;
	/* bandwidths for each window in time */
	float read_mbs,send_mbs,recv_mbs,write_mbs;

	/* try to open file */
	if (strlen(outfilename)==0) {
		outfile = stdout;
	} else {
		outfile = fopen(outfilename, "w");
	}
	/* do we have a file pointer? */
	if (outfile == NULL) {
		fprintf(stderr,"Can not open output file: %s\n",outfilename);
		exit(1);
	}

	/* how many qthreads are there? */
	int total_threads = 0;
	for (i = 0; i < src->tt_size; i++) {
		total_threads = MAX(src->tte[i].qthread_number,total_threads);
	}
	total_threads += 1;

	/* file header */
	fprintf(outfile,"#timestamp: %s",src->td);
	fprintf(outfile,"#reqsize: %d\n",src->reqsize);
	fprintf(outfile,"#filesize: %lld\n",src->reqsize*src->tt_size);
        fprintf(outfile,"#qthreads_src, target pid, pids: %d %d ",src->target_thread_id,total_threads_src);
        for (i = 0; i < total_threads_src; i++) { fprintf(outfile,"%d ",thread_id_src[i] );}
                                                  fprintf(outfile,"\n");
        fprintf(outfile,"#qthreads_dst, target pid, pids: %d %d ",dst->target_thread_id,total_threads_dst);
        for (i = 0; i < total_threads_dst; i++) { fprintf(outfile,"%d ",thread_id_dst[i] );}
                                                  fprintf(outfile,"\n");
        fprintf(outfile,"#src_start_norm, dst_start_norm %lld %lld\n",src_start_norm,dst_start_norm);
	fprintf(outfile,"#read_time      read_bw  send_time      send_bw  recv_time      recv_bw  write_time     write_bw  time_unit  bw_unit\n");
//        fprintf(outfile,"#opnum,    tid,      read_time, opnum,    tid,  #ops,   #bytes,      send_time, opnum,    tid,  #ops,   #bytes,      recv_time, opnum,    tid,     write_time\n");

	/* loop through tte entries */
	for (i = 0; i < src->tt_size; i++) {
		/* make sure this is an op that matters */
		read_mbs = send_mbs = recv_mbs = write_mbs = 0.0;

		/* read disk */
		cutoff = MAX( nclk2sec(read_op[i]->disk_end)-window_size, 0.0);
		for (k = i; (k >= 0) && ( nclk2sec(read_op[k]->disk_end) > cutoff); k--)
			read_mbs  += (float)read_op[k]->disk_xfer_size / BPU;
		/* send net */
		cutoff = MAX( nclk2sec(send_op[i]->net_end)-window_size ,0.0);
		for (k = i; (k >= 0) && ( nclk2sec(send_op[k]->net_end) > cutoff); k--) 
			send_mbs  += (float)send_op[k]->net_xfer_size / BPU;
		/* recv net */
		cutoff = MAX( nclk2sec(recv_op[i]->net_end)-window_size, 0.0);
		for (k = i; (k >= 0) && ( nclk2sec(recv_op[k]->net_end) > cutoff); k--) 
			recv_mbs  += (float)recv_op[k]->net_xfer_size / BPU;
		/* write disk */
		cutoff = MAX( nclk2sec(write_op[i]->disk_end)-window_size, 0.0);
		for (k = i; (k >= 0) && (nclk2sec(write_op[k]->disk_end) > cutoff); k--) 
			write_mbs += (float)write_op[k]->disk_xfer_size / BPU;

		/* divide by the time in the sliding window */
		read_mbs  /= (float)window_size;
		send_mbs  /= (float)window_size;
		recv_mbs  /= (float)window_size;
		write_mbs /= (float)window_size;

		/* write to file */
                char line[256] = {" "};
		sprintf(line,"%12.6f %10.4f %12.6f %10.4f %12.6f %10.4f  %12.6f %10.4f  %9s %8s\n",
			nclk2sec(read_op[i]->disk_end), read_mbs,
			nclk2sec(send_op[i]->net_end), send_mbs,
			nclk2sec(recv_op[i]->net_end), recv_mbs,
			nclk2sec(write_op[i]->disk_end), write_mbs,
			"s", "MB/s");
                if (read_op[i]->disk_end  == 0 ) strncpy(&line[3],"?",1); 
                if (read_op[i]->disk_end  == 0 ) strncpy(&line[16],"?",1); 
                if (write_op[i]->disk_end == 0 ) strncpy(&line[76],"?",1);
                if (write_op[i]->disk_end == 0 ) strncpy(&line[89],"?",1);
		fprintf(outfile,"%s",line);
	}
	fclose(outfile);
}

/* write the outfile kernel */
void write_outfile_k(tthdr_t *src, tthdr_t *dst, tte_t **read_op,
		tte_t **send_op, tte_t **recv_op, tte_t **write_op) {
	int64_t i,k;
	float cutoff;
	/* variables for the file writing loop below */
	FILE *outfile;
	/* bandwidths for each window in time */
	float read_mbs,send_mbs,recv_mbs,write_mbs;

	/* try to open another file */
                strcat(outfilename,".k");
		outfile = fopen(outfilename, "w");
	/* do we have a file pointer? */
	if (outfile == NULL) {
		fprintf(stderr,"Can not open output file: %s\n",outfilename);
		exit(1);
        }

	/* file header */
        fprintf(outfile,"#timestamp: %s",src->td);
        fprintf(outfile,"#reqsize: %d\n",src->reqsize);
        fprintf(outfile,"#filesize: %lld\n",src->reqsize*src->tt_size);
        fprintf(outfile,"#qthreads_src, target pid, pids: %d %d ",src->target_thread_id,total_threads_src);
        for (i = 0; i < total_threads_src; i++) { fprintf(outfile,"%d ",thread_id_src[i] );}
                                                  fprintf(outfile,"\n");
        fprintf(outfile,"#qthreads_dst, target pid, pids: %d %d ",dst->target_thread_id,total_threads_dst);
        for (i = 0; i < total_threads_dst; i++) { fprintf(outfile,"%d ",thread_id_dst[i] );}
                                                  fprintf(outfile,"\n");
        fprintf(outfile,"#src_start_norm, dst_start_norm %lld %lld\n",src_start_norm,dst_start_norm);
	fprintf(outfile,"#read_time      read_bw  send_time      send_bw  recv_time      recv_bw  write_time     write_bw  time_unit  bw_unit\n");

	/* loop through tte entries */
	for (i = 0; i < src->tt_size; i++) {
		/* make sure this is an op that matters */
		read_mbs = send_mbs = recv_mbs = write_mbs = 0.0;

		/* read disk */
		cutoff = MAX( nclk2sec(read_op[i]->disk_end_k)-window_size, 0.0);
		for (k = i; (k >= 0) && ( nclk2sec(read_op[k]->disk_end_k) > cutoff); k--)
			read_mbs  += (float)read_op[k]->disk_xfer_size / BPU;
		/* send net */
		cutoff = MAX( nclk2sec(send_op[i]->net_end_k)-window_size ,0.0);
		for (k = i; (k >= 0) && ( nclk2sec(send_op[k]->net_end_k) > cutoff); k--) 
			send_mbs  += (float)send_op[k]->net_xfer_size / BPU;
		/* recv net */
		cutoff = MAX( nclk2sec(recv_op[i]->net_end_k)-window_size, 0.0);
		for (k = i; (k >= 0) && ( nclk2sec(recv_op[k]->net_end_k) > cutoff); k--) 
			recv_mbs  += (float)recv_op[k]->net_xfer_size / BPU;
		/* write disk */
		cutoff = MAX( nclk2sec(write_op[i]->disk_end_k)-window_size, 0.0);
		for (k = i; (k >= 0) && (nclk2sec(write_op[k]->disk_end_k) > cutoff); k--) 
			write_mbs += (float)write_op[k]->disk_xfer_size / BPU;

		/* divide by the time in the sliding window */
		read_mbs    /= (float)window_size;
		send_mbs    /= (float)window_size;
		recv_mbs    /= (float)window_size;
		write_mbs   /= (float)window_size;

		/* write to file. use same time scale as write_outfile for comparison */
                char line[256] = {" "};
		sprintf(line,"%12.6f %10.4f %12.6f %10.4f %12.6f %10.4f  %12.6f %10.4f  %9s %8s\n",
			nclk2sec(read_op[i]->disk_end), read_mbs,
			nclk2sec(send_op[i]->net_end), send_mbs,
			nclk2sec(recv_op[i]->net_end), recv_mbs,
			nclk2sec(write_op[i]->disk_end), write_mbs,
			"s", "MB/s");
                if (read_op[i]->disk_end  == 0 ) strncpy(&line[3],"?",1); 
                if (read_op[i]->disk_end  == 0 ) strncpy(&line[16],"?",1); 
                if (write_op[i]->disk_end == 0 ) strncpy(&line[76],"?",1);
                if (write_op[i]->disk_end == 0 ) strncpy(&line[89],"?",1);
		fprintf(outfile,"%s",line);
	}
	fclose(outfile);
}

/* write the outfile delta xdd-kernel */
void write_outfile_d(tthdr_t *src, tthdr_t *dst, tte_t **read_op,
		tte_t **send_op, tte_t **recv_op, tte_t **write_op) {
	int64_t i;
	/* variables for the file writing loop below */
	FILE *outfile;

	/* try to open another file */
                strcat(outfilename,".d");
		outfile = fopen(outfilename, "w");
	/* do we have a file pointer? */
	if (outfile == NULL) {
		fprintf(stderr,"Can not open output file: %s\n",outfilename);
		exit(1);
        }

	/* file header */
        fprintf(outfile,"#timestamp: %s",src->td);
        fprintf(outfile,"#reqsize: %d\n",src->reqsize);
        fprintf(outfile,"#filesize: %lld\n",src->reqsize*src->tt_size);
        fprintf(outfile,"#qthreads_src, target pid, pids: %d %d ",src->target_thread_id,total_threads_src);
        for (i = 0; i < total_threads_src; i++) { fprintf(outfile,"%d ",thread_id_src[i] );}
                                                  fprintf(outfile,"\n");
        fprintf(outfile,"#qthreads_dst, target pid, pids: %d %d ",dst->target_thread_id,total_threads_dst);
        for (i = 0; i < total_threads_dst; i++) { fprintf(outfile,"%d ",thread_id_dst[i] );}
                                                  fprintf(outfile,"\n");
        fprintf(outfile,"#src_start_norm, dst_start_norm %lld %lld\n",src_start_norm,dst_start_norm);
	fprintf(outfile,"#   read_time read_start   read_end   send_time  send_start   send_end");
	fprintf(outfile,"#   recv_time recv_start   recv_end   write_time write_start  write_end\n");
	fprintf(outfile,"#             (kern-xdd)   (xdd-kern)            (kern-xdd)   (xdd-kern)");
	fprintf(outfile,"#             (kern-xdd)   (xdd-kern)            (kern-xdd)   (xdd-kern)\n");

	/* loop through tte entries */
	for (i = 0; i < src->tt_size; i++) {
		/* write to file */
                char line[256] = {" "};
		sprintf(line,"%12.6f %10lld %10lld %12.6f %10lld %10lld %12.6f %10lld %10lld %12.6f %10lld %10lld\n",
		 nclk2sec(read_op[i]->disk_end),  read_op[i]->disk_start_k  - read_op[i]->disk_start, 
		                                  read_op[i]->disk_end      - read_op[i]->disk_end_k, 
		 nclk2sec(send_op[i]->net_end),   send_op[i]->net_start_k   - send_op[i]->net_start,
		                                  send_op[i]->net_end       - send_op[i]->net_end_k,
		 nclk2sec(recv_op[i]->net_end),   recv_op[i]->net_start_k   - recv_op[i]->net_start, 
		                                  recv_op[i]->net_end       - recv_op[i]->net_end_k, 
		 nclk2sec(write_op[i]->disk_end), write_op[i]->disk_start_k - write_op[i]->disk_start,
		                                  write_op[i]->disk_end     - write_op[i]->disk_end_k
			);
                if (read_op[i]->disk_end  == 0 ) strncpy(&line[3],"?",1); 
                if (read_op[i]->disk_end  == 0 ) strncpy(&line[21],"?",1); 
                if (read_op[i]->disk_end  == 0 ) strncpy(&line[32],"?",1); 
                if (write_op[i]->disk_end == 0 ) strncpy(&line[108],"?",1);
                if (write_op[i]->disk_end == 0 ) strncpy(&line[126],"?",1);
                if (write_op[i]->disk_end == 0 ) strncpy(&line[137],"?",1);
		fprintf(outfile,"%s",line);
	}
	fclose(outfile);
}

/*****************************************************
 * Reads file1 and file2, then returns the respective
 * source and destination structures.
 *****************************************************/
int xdd_getdata(char *file1, char *file2, tthdr_t **src, tthdr_t **dst, size_t *tsdata_size) {

	char *filename = NULL;
	tthdr_t *tsdata = NULL;

	/* read file1 and file2 */
	char i;
	for (i=0; i < 2; i++) {
		if (i == 0)
			filename = file1;
		else if (i == 1)
			filename = file2;

		/* read xdd timestamp dump */
		int retval = xdd_readfile(filename, &tsdata, tsdata_size);
		if (retval == 0) {
			fprintf(stderr,"Failed to read binary XDD timestamp dump: %s\n",filename);
			return 0;
		}

		/* no empty sets */
		if (tsdata->tt_size < 1) {
			fprintf(stderr,"Timestamp dump was empty: %s\n",filename);
			return 0;
		}

		/* determine whether this is the source or destination file */
		if (READ_OP(tsdata->tte[0].op_type)) {
			if (*src != NULL) {
				fprintf(stderr,"You passed 2 source dump files.\n");
				fprintf(stderr,"Please pass matching source and destination dumps.\n");
				return 0;
			}
			*src = tsdata;
		} else if (WRITE_OP(tsdata->tte[0].op_type)) {
			if (*dst != NULL) {
				fprintf(stderr,"You passed 2 destination dump files.\n");
				fprintf(stderr,"Please pass matching source and destination dumps.\n");
				return 0;
			}
			*dst = tsdata;
		} else {
			fprintf(stderr,"Timestamp dump had invalid operation in tte[0]: %s\n",filename);
			return 0;
		}

		tsdata = NULL;
		filename = NULL;
	}

	/* If these files are from the same transfer,
	 * they should be the same size. */
	if ((*src)->tt_bytes != (*dst)->tt_bytes) {
		fprintf(stderr,"Source and destination files are not the same size.\n");
		fprintf(stderr,"Please pass matching source and destination dumps.\n");
		return 0;
	}

	return 1;
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
int xdd_readfile(char *filename, tthdr_t **tsdata, size_t *tsdata_size) {

	FILE *tsfd;
	size_t tsize = 0;
	tthdr_t *tdata = NULL;
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

	/* check magic number in tthdr */
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
int xdd_writefile(char *filename, tthdr_t *tsdata, size_t tsdata_size) {

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
	strcpy(outfilename,"");
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
				strncpy(outfilename,optarg,OUTFILENAME_LEN);
				outfilename[OUTFILENAME_LEN-1] = '\0';
				argnum += 2;
				if (strlen(outfilename) == 0)
					ierr++;
				break;
			case 'h': /* help */
			default:
				printusage(argv[0]);
				break;
		}
	}

	/* do we have two filenames and no parsing errors? */
	if ( (argc-argnum != 2) || (ierr != 0) ) {
		printusage(argv[0]);
	}

	/* return the number of the first non-option argument */
	return argnum;
}


/* prints some help text and exits */
void printusage(char *progname) {
	fprintf(stderr, "\n");
	fprintf(stderr, "USAGE: %s [OPTIONS] FILE FILE\n\n",progname);

	fprintf(stderr, "This program takes 2 XDD timestamp dump files (source and dest)\n");
	fprintf(stderr, "from a file transfer, and determines the approximate bandwidth at\n");
	fprintf(stderr, "any given time by using a sliding window approach.\n\n");

	fprintf(stderr, "For instance, if you pass '-t 5' then it will set the window size\n");
	fprintf(stderr, "to 5 seconds.  At each point in time during the transfer, it will\n");
	fprintf(stderr, "calculate the amount of data committed during the past 5 seconds.\n");
	fprintf(stderr, "It outputs the bandwidth during that window, so you can plot the\n");
	fprintf(stderr, "bandwidth as a function of running time for each operation.\n");
	fprintf(stderr, "The operations consist of:\n");
	fprintf(stderr, "   read (src) -> send (src) -> recv (dst) -> write (dst)\n\n");

	fprintf(stderr, "To get an XDD timestamp dump, use the '-ts dump FILE' option of XDD.\n");
	fprintf(stderr, "In XDDCP, you can just pass the -w flag.\n\n");

	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "\t -h            \t print this usage text\n");
	fprintf(stderr, "\t -o <filename> \t output filename (default: stdout)\n");
	fprintf(stderr, "\t -t <seconds>  \t number of seconds in the sliding window (default: %d)\n", window_size);

	fprintf(stderr, "\n");
	exit(1);
}


