
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "pclk.h"
#include "xdd.h"

/* make sure MAX is defined */
#ifndef MAX
#define MAX(a,b) (a > b ? a : b)
#endif
#ifndef MIN
#define MIN(a,b) (a < b ? a : b)
#endif
/* converts xdd pclk time to seconds, given:
 * p (pclk value) and r (timer resolution) */
#define pclk2sec(p,r) ((float)p/(float)r/1.0e9)
/* Bytes Per Unit... don't set to 0.  1000000 is MB/s */
#define BPU (1000000.0)

/* magic number to make sure we can read the file */
#define BIN_MAGIC_NUMBER 0xDEADBEEF

/* output file, if specified */
#define OUTFILENAME_LEN 512
char outfilename[OUTFILENAME_LEN];

/* number of seconds in the sliding window */
int window_size;

/* sorts the timestamp entries by completion time *
void sort_by_time(tthdr_t *src, tthdr_t *dst, tte_t ***read_op,
                  tte_t ***send_op, tte_t ***recv_op, tte_t ***write_op);
/* write all the outfiles *
void write_outfile(tthdr_t *src, tthdr_t *dst, tte_t **read_op,
                   tte_t **send_op, tte_t **recv_op, tte_t **write_op);
/* read, check, and store the src and dst file data *
int xdd_getdata(char *file1, char *file2, tthdr_t **src, tthdr_t **dst);
/* Read an XDD binary timestamp dump into a structure *
int xdd_readfile(char *filename, tthdr_t **tsdata, size_t *tsdata_size);
/* parse command line options */
int getoptions(int argc, char **argv);
/* print command line usage */
void printusage(char *progname);


int main(int argc, char **argv) {

    int fn, argnum, num_dumps, i;

    /* tsdumps for the source and destination sides */
    tthdr_t **tsdumps = NULL;
    tte_t ***disk_ops = NULL;
    tte_t ***net_ops = NULL;
    
    /* get command line options */
    argnum = getoptions(argc, argv);
    fn = MAX(argnum,1);
    num_dumps = argc - fn;

    /* Allocate space for timestamps */
    tsdumps = malloc(sizeof(tthdr_t*) * num_dumps);
    disk_ops = malloc(sizeof(tte_t**) * num_dumps);    
    net_ops = malloc(sizeof(tte_t**) * num_dumps);    

    /* get the src and dst data structs */
    for (i = 0; i < num_dumps; i++) {

        int rc = xdd_getdata(argv[fn + i], tsdumps + i);
        if (0 != rc) {
             fprintf(stderr,"xdd_getdata() failed... exiting.\n");
             exit(1);
        }

        /* Sort timestamp output */
        sort_by_time(tsdumps + i, disk_ops + i, net_ops + i);
    }

    //write_outfile(num_dumps, tsdumps, disk_ops, net_ops);    
    return 0;
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
    while ((opt = getopt(argc, argv, "t:o:h")) != -1) {
        switch (opt) {
            case 't': /* moving average */
                window_size = atoi(optarg);
                argnum += 2;
                if (window_size <= 0) {
                    fprintf(stderr,"\nwindow size must be more than 0.\n\n");
                    ierr++;
                }
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
    if ( (argc-argnum < 1) || (ierr != 0) ) {
        printusage(argv[0]);
    }

    /* return the number of the first non-option argument */
    return argnum;
}


/* prints some help text and exits */
void printusage(char *progname) {
    fprintf(stderr, "\n");
    fprintf(stderr, "USAGE: %s [OPTIONS] FILE...\n\n",progname);

    fprintf(stderr, "This program takes XDD timestamp dump files and\n");
    fprintf(stderr, "computes the approximate bandwidth at any given time by\n");
    fprintf(stderr, "using a sliding window approach.\n\n");

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

/*****************************************************
 * Reads file1 and file2, then returns the respective
 * source and destination structures.
 *****************************************************/
int xdd_getdata(char *filename, tthdr_t **src) {

    size_t tsdata_size = 0;

    /* read xdd timestamp dump */
    int retval = xdd_readfile(filename, src, &tsdata_size);
    if (retval != 0) {
        fprintf(stderr,"Failed to read binary XDD timestamp dump: %s\n",filename);
        return 1;
    }

    /* no empty sets */
    if (tsdata_size < 1) {
        fprintf(stderr,"Timestamp dump was empty: %s\n",filename);
        return 1;
    }
    
    return 0;
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
 *   0 if succeeded, 1 if failed
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
        return 1;
    }

    /* check magic number in tthdr */
    result = fread(&magic,sizeof(uint32_t),1,tsfd);
    fseek(tsfd,0,SEEK_SET);

    if (result == 0) {
        fprintf(stderr,"Error reading file: %s\n",filename);
        return 1;
    }
    if (magic != BIN_MAGIC_NUMBER) {
        fprintf(stderr,"File is not in a readable format: %s\n",filename);
        return 1;
    }

    /* read rest of structure */
    result = fread(tdata,tsize,1,tsfd);

    if (result == 0) {
        fprintf(stderr,"Error reading file: %s\n",filename);
        return 1;
    }

    *tsdata = tdata;
    *tsdata_size = tsize;

    /* close file */
    fclose(tsfd);

    return 0;
}

/** Compare timestamp entries by disk end time */
int compare_by_disk_end(const void* lhs, const void* rhs) {
    pclk_t* ltime = (pclk_t*)((char*)lhs + offsetof(tte_t, disk_end));
    pclk_t* rtime = (pclk_t*)((char*)rhs + offsetof(tte_t, disk_end));
    
    int cmp = 0;
    if (*ltime < *rtime) {
        cmp = -1;
    }
    else if (*ltime > *rtime) {
        cmp = 1;
    }
    printf("Disk end times: %lld %lld\n", *ltime, *rtime);
    return cmp;
}

/** Compare timestamp entries by net end time */
int compare_by_net_end(const void* lhs, const void* rhs) {
    pclk_t* ltime = (pclk_t*)((char*)lhs + offsetof(tte_t, net_end));
    pclk_t* rtime = (pclk_t*)((char*)rhs + offsetof(tte_t, net_end));

    int cmp = 0;
    if (*ltime < *rtime) {
        cmp = -1;
    }
    else if (*ltime > *rtime) {
        cmp = 1;
    }
    return cmp;
}

/* sort timestamp entries by completion time */
void sort_by_time(tthdr_t *src, tte_t **disk_ops, tte_t** net_ops) {

    size_t num_entries = src->tt_size;
    
    /* allocate array of pointers to tte structs */
    disk_ops = malloc(num_entries * sizeof(tte_t*));
    net_ops = malloc(num_entries * sizeof(tte_t*));

    if (disk_ops == NULL || net_ops == NULL) {
        fprintf(stderr,"Could not allocate enough memory for quick sort.\n");
        exit(1);
    }

    /* initialize pointers */
    int64_t i;
    for (i = 0; i < src->tt_size; i++) {
        disk_ops[i] = &(src->tte[i]);
        net_ops[i] = &(src->tte[i]);
    }

    /* sort pointers */
    qsort(disk_ops, src->tt_size, sizeof(tte_t*), compare_by_disk_end);
    qsort(net_ops, src->tt_size, sizeof(tte_t*), compare_by_net_end);
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
    fprintf(outfile,"#filesize: %ld\n",src->reqsize*src->tt_size);
    fprintf(outfile,"#qthreads: %d\n",total_threads);
    fprintf(outfile,"#read_time    read_bw  send_time    send_bw  recv_time    recv_bw  write_time   write_bw  time_unit  bw_unit\n");

    /* loop through tte entries */
    for (i = 0; i < src->tt_size; i++) {
        /* make sure this is an op that matters */
        read_mbs = send_mbs = recv_mbs = write_mbs = 0.0;

        /* read disk */
        cutoff = MAX( pclk2sec(read_op[i]->disk_end,src->res)-window_size, 0.0);
        for (k = i; (k >= 0) && ( pclk2sec(read_op[k]->disk_end,src->res) > cutoff); k--)
            read_mbs  += (float)read_op[k]->disk_xfer_size / BPU;
        /* send net */
        cutoff = MAX( pclk2sec(send_op[i]->net_end,src->res)-window_size ,0.0);
        for (k = i; (k >= 0) && ( pclk2sec(send_op[k]->net_end,src->res) > cutoff); k--) 
            send_mbs  += (float)send_op[k]->net_xfer_size / BPU;
        /* recv net */
        cutoff = MAX( pclk2sec(recv_op[i]->net_end,dst->res)-window_size, 0.0);
        for (k = i; (k >= 0) && ( pclk2sec(recv_op[k]->net_end,dst->res) > cutoff); k--) 
            recv_mbs  += (float)recv_op[k]->net_xfer_size / BPU;
        /* write disk */
        cutoff = MAX( pclk2sec(write_op[i]->disk_end,dst->res)-window_size, 0.0);
        for (k = i; (k >= 0) && (pclk2sec(write_op[k]->disk_end,dst->res) > cutoff); k--) 
            write_mbs += (float)write_op[k]->disk_xfer_size / BPU;

        /* divide by the time in the sliding window */
        read_mbs  /= (float)window_size;
        send_mbs  /= (float)window_size;
        recv_mbs  /= (float)window_size;
        write_mbs /= (float)window_size;

        /* write to file */
        fprintf(outfile,"%10.4f %10.4f %10.4f %10.4f %10.4f %10.4f  %10.4f %10.4f  %9s %8s\n",
                pclk2sec(read_op[i]->disk_end,src->res), read_mbs,
                pclk2sec(send_op[i]->net_end,src->res), send_mbs,
                pclk2sec(recv_op[i]->net_end,src->res), recv_mbs,
                pclk2sec(write_op[i]->disk_end,src->res), write_mbs,
                "s", "MB/s");
    }
    fclose(outfile);
}


/*
 * Local variables:
 *  indent-tabs-mode: t
 *  c-indent-level: 4
 *  c-basic-offset: 4
 * End:
 *
 * vim: ts=4 sts=4 sw=4 noexpandtab
 */

