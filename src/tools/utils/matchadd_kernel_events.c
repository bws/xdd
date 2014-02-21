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
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#define XDDMAIN
#include "xint.h"

#define DEBUG 0

/* returns 1 if x is expected operation... 0 if not */
#define READ_OP(x)  (x==TASK_OP_TYPE_READ||x==SO_OP_READ)
#define WRITE_OP(x) (x==TASK_OP_TYPE_WRITE||x==SO_OP_WRITE||x==SO_OP_WRITE_VERIFY)
#define NO_OP(x)    (x==TASK_OP_TYPE_NOOP||x==SO_OP_NOOP)
#define EOF_OP(x)   (x==TASK_OP_TYPE_EOF||x==SO_OP_EOF)

//
//
/**
 *  Parse and interpret lines
 */
void parse_line (
                 int thread_pid,
                 char *cmdline,
                 char *operation,
                 int  *found,
		 int64_t *ts_beg_op,
		 int64_t *ts_end_op,
		 int64_t *size_op,
		 int64_t *nops_op
                )
{
  static char *seps = " ,;\n\t{}";
  char *keyword;
  int64_t secs, nsecs, ts_enter, ts_exit, size_exit;
  static int64_t  sum_size_op, size_enter;

  /* Perform some initialization that I have no idea if its correct */
  secs = 0;
  ts_exit = 0;
 
  if ( strlen(cmdline) == 0  || cmdline[0]=='#' ) return;

  keyword = strtok(cmdline,seps);
  if ( keyword == NULL )
  {
      fprintf( stderr, "Warning: Unrecognized line: <%s>\n", cmdline );
      return;
  }

/* enter */
  if ( !strncmp(keyword,"enter",5) ) 
  {
    keyword = strtok(NULL,seps);
    if ( !strcmp(keyword,operation) ) 
    {
    while ( (keyword = strtok(NULL,seps)) != NULL)
    {
      if ( !strncmp(keyword,"pid" ,  3 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        if ( atoi(keyword) != thread_pid) return;
        if(DEBUG)fprintf(stderr,"enter %s pid= %d ",operation,thread_pid);
      }
      if ( !strncmp(keyword,"secs" , 4 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        secs = atoll(keyword);
        if(DEBUG)fprintf(stderr,"secs= %"PRId64" ",secs);
      }
      if ( !strncmp(keyword,"nsecs" , 5 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        nsecs = atoll(keyword);
        ts_enter = secs * 1000000000LL + nsecs;
        if (*nops_op == 0 ) 
        {
          *ts_beg_op  = ts_enter;
          sum_size_op = 0;
          size_enter = 0;
        }
        if(DEBUG)fprintf(stderr," nsecs= %"PRId64" ",nsecs);
      }
      if ( !strcmp(keyword,"size") || 
           !strcmp(keyword,"count")|| 
           !strcmp(keyword,"len") ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        size_enter = atoi(keyword);
        if (*nops_op == 0 ) *size_op = size_enter;
        if(DEBUG)fprintf(stderr," bytes= %"PRId64" ",size_enter);
      }
    }
    if (DEBUG) fprintf(stderr,"cr-n\n");
  }
 }
/* exit */
  else
  if ( !strncmp(keyword,"exit",5) ) 
  {
    keyword = strtok(NULL,seps);
    if ( !strcmp(keyword,operation) ) 
    {
    while ( (keyword = strtok(NULL,seps)) != NULL)
    {
      if ( !strncmp(keyword,"pid" ,  3 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        if ( atoi(keyword) != thread_pid) return;
       if(DEBUG) fprintf(stderr,"exit %s pid= %d ",operation,thread_pid);
      }
      if ( !strncmp(keyword,"secs" , 4 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        secs = atoi(keyword);
        if (DEBUG) fprintf(stderr,"secs= %"PRId64" ",secs);
      }
      if ( !strncmp(keyword,"nsecs" , 5 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        nsecs = atoi(keyword);
        ts_exit = secs * 1000000000LL + nsecs;
       if(DEBUG) fprintf(stderr," nsecs= %"PRId64" ",nsecs);
      }
      if ( !strncmp(keyword,"ret" , 3 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        size_exit = atoi(keyword);
        sum_size_op += size_exit;
        (*nops_op)++;
       if(DEBUG) fprintf(stderr," bytes= %"PRId64" sum %"PRId64" of %"PRId64" nops %"PRId64,size_exit, sum_size_op, *size_op, *nops_op);
        /* check for end-of-op */
        if (size_exit == size_enter)
        {
          if ( sum_size_op != *size_op ) 
          {
            fprintf(stderr,"sum=%"PRId64", size=%"PRId64" DONT match reject\n",sum_size_op, *size_op);
                *ts_beg_op = *ts_end_op = *size_op = *nops_op = 0;
              return;
          }
         *ts_end_op = ts_exit;
         *found = 1;
          if (DEBUG)
          fprintf(stderr,"kern %8s thread_pid %d size %8"PRId64"  start %"PRId64" end %"PRId64", sum %"PRId64", nops %"PRId64"\n",
             operation, thread_pid, *size_op, (*ts_beg_op),(*ts_end_op), sum_size_op, *nops_op);
        }
      }
    }
    if (DEBUG) fprintf(stderr,"cr-x\n");
  }
 }
  return;
}


/**
 *  Replace newlines with 0 \n
 *
 *  \param str String to Modify
 */
static void chop(char *str) {
  do {
    if (*str=='\n') *str= 0;
  } while (*++str);
}

/**
 *  Parse file from current position for next operation, if any \n
 *
 */
int  parse_file_next_op (
                   int thread_pid,
                   FILE *file,
                   char *operation,
                   int64_t *ts_beg,
                   int64_t *ts_end,
                   int64_t *size_op,
                   int64_t *nops_op
                  ) 
{
  char 
     line[1024];

  int 
    found = 0;

  while (fgets(line,1024,file) != NULL)
  {
    chop(line);
    parse_line(thread_pid,line,operation,&found,ts_beg,ts_end,size_op,nops_op);
    if (found) break;
  }

  return (0);
}

int 
matchadd_kernel_events(int issource, int nthreads, int thread_id[], char *filespec, xdd_ts_header_t *xdd_data)
{
    size_t i;
    int k;
           FILE *fp;
           int64_t ts_beg_op, ts_end_op, size_op, nops_op;

  /* --------------------- */
  /* Executable Statements */
  /* --------------------- */

         fp = fopen( filespec, "r");
         if ( fp == NULL )
         {
           fprintf(stderr,"matchadd_kernel_events: error: can't open file: %s\n",filespec);
           return (-1);
         }
         for (k = 0; k < nthreads; k++ )
         {
           /* each thread in chrono order; rewind kernel file for each pid */
           fseek(fp,0,SEEK_SET);
           /* for thread k, find matching kernel events and insert in xdd_data struct */
           for (i = 0; i < xdd_data->tsh_tt_size; i++)
           {
            
            if (xdd_data->tsh_tte[i].tte_thread_id == thread_id[k])
            {
              if (issource)
              {
              /* find next op. Deal with xdd eofi, i.e., don't look for trace events that did not happen */
                if (DEBUG)
                fprintf(stderr,"%2zd xdd  pread64  thread_pid %d size %8d  start %lld end %lld opt %d\n",
                   i,xdd_data->tsh_tte[i].tte_thread_id, xdd_data->tsh_tte[i].tte_disk_xfer_size, xdd_data->tsh_tte[i].tte_disk_start, xdd_data->tsh_tte[i].tte_disk_end,xdd_data->tsh_tte[i].tte_op_type );
                if (READ_OP(xdd_data->tsh_tte[i].tte_op_type) && xdd_data->tsh_tte[i].tte_disk_xfer_size)
                {
                  ts_beg_op = ts_end_op = size_op = nops_op = 0;
                  parse_file_next_op(thread_id[k],fp,"pread64",&ts_beg_op,&ts_end_op,&size_op,&nops_op);
                  xdd_data->tsh_tte[i].tte_disk_start_k     = ts_beg_op;
                  xdd_data->tsh_tte[i].tte_disk_end_k       = ts_end_op;
                  if ( xdd_data->tsh_tte[i].tte_disk_xfer_size != size_op) 
                    fprintf(stderr, "xddop# %zd pid %d size %d != %"PRId64"\n",i,thread_id[k],xdd_data->tsh_tte[i].tte_disk_xfer_size,size_op);
                }
                if ( xdd_data->tsh_tte[i].tte_net_xfer_size > 0 )
                {
                  ts_beg_op = ts_end_op = size_op   = nops_op = 0;
                if (DEBUG)
                  fprintf(stderr,"%2zd xdd  sendto   thread_pid %d size %7d  start %lld end %lld opt %d\n",
                     i,xdd_data->tsh_tte[i].tte_thread_id, xdd_data->tsh_tte[i].tte_net_xfer_size, xdd_data->tsh_tte[i].tte_net_start, xdd_data->tsh_tte[i].tte_net_end,xdd_data->tsh_tte[i].tte_op_type );
                  parse_file_next_op(thread_id[k],fp,"sendto",&ts_beg_op,&ts_end_op,&size_op,&nops_op);
                  xdd_data->tsh_tte[i].tte_net_start_k      = ts_beg_op;
                  xdd_data->tsh_tte[i].tte_net_end_k        = ts_end_op;
                  if ( xdd_data->tsh_tte[i].tte_net_xfer_size != size_op && size_op > 0) 
                    fprintf(stderr, "xddop# %zd pid %d size %d != %"PRId64"\n",i,thread_id[k],xdd_data->tsh_tte[i].tte_net_xfer_size,size_op);
                  if ( xdd_data->tsh_tte[i].tte_net_xfer_calls != nops_op && nops_op > 0) 
                    fprintf(stderr, "xddop# %zd pid %d nops %d != %"PRId64"\n",i,thread_id[k],xdd_data->tsh_tte[i].tte_net_xfer_calls,nops_op);
                }
              }
              else /* is destination, parse for net op first (recvfrom) */
              {
                /* parse for net op only if there was an app net op */
                if ( xdd_data->tsh_tte[i].tte_net_xfer_size > 0 )
                {
                if (DEBUG)
                  fprintf(stderr,"%2zd xdd  recvfrom thread_pid %d size %7d  start %lld end %lld opt %d\n",
                     i,xdd_data->tsh_tte[i].tte_thread_id, xdd_data->tsh_tte[i].tte_net_xfer_size, xdd_data->tsh_tte[i].tte_net_start, xdd_data->tsh_tte[i].tte_net_end,xdd_data->tsh_tte[i].tte_op_type );
                  ts_beg_op = ts_end_op = size_op = nops_op = 0;
                  parse_file_next_op(thread_id[k],fp,"recvfrom",&ts_beg_op,&ts_end_op,&size_op,&nops_op);
                  xdd_data->tsh_tte[i].tte_net_start_k      = ts_beg_op;
                  xdd_data->tsh_tte[i].tte_net_end_k        = ts_end_op;
                  /* xdd eof stuff */
                  if ( xdd_data->tsh_tte[i].tte_net_xfer_size != size_op && xdd_data->tsh_tte[i].tte_net_xfer_size != sizeof(xdd_e2e_header_t)) 
                    fprintf(stderr, "xddop# %zd pid %d op %d size %d != %"PRId64"\n",i,thread_id[k],xdd_data->tsh_tte[i].tte_op_type,xdd_data->tsh_tte[i].tte_net_xfer_size,size_op);
                  if ( xdd_data->tsh_tte[i].tte_net_xfer_calls != nops_op && nops_op > 0) 
                    fprintf(stderr, "xddop# %zd pid %d nops %d != %"PRId64"\n",i,thread_id[k],xdd_data->tsh_tte[i].tte_net_xfer_calls,nops_op);
                }
                if (DEBUG)
                fprintf(stderr,"%2zd xdd  pwrite64 thread_pid %d size %8d  start %lld end %lld opt %d\n",
                   i,xdd_data->tsh_tte[i].tte_thread_id, xdd_data->tsh_tte[i].tte_disk_xfer_size, xdd_data->tsh_tte[i].tte_disk_start, xdd_data->tsh_tte[i].tte_disk_end,xdd_data->tsh_tte[i].tte_op_type );
                if (WRITE_OP(xdd_data->tsh_tte[i].tte_op_type) && xdd_data->tsh_tte[i].tte_disk_xfer_size)
                {
                  ts_beg_op = ts_end_op = size_op = nops_op = 0;
                  parse_file_next_op(thread_id[k],fp,"pwrite64",&ts_beg_op,&ts_end_op,&size_op,&nops_op);
                  xdd_data->tsh_tte[i].tte_disk_start_k     = ts_beg_op;
                  xdd_data->tsh_tte[i].tte_disk_end_k       = ts_end_op;
                  if ( xdd_data->tsh_tte[i].tte_disk_xfer_size != size_op && size_op > 0) 
                  fprintf(stderr, "xddop# %zd pid %d op %d size %d != %"PRId64"\n",i,thread_id[k],xdd_data->tsh_tte[i].tte_op_type,xdd_data->tsh_tte[i].tte_disk_xfer_size,size_op);
                }
              }
            }
           }
          }
           if (DEBUG)
           for (i = 0; i < xdd_data->tsh_tt_size; i++)
           {
              fprintf(stderr,"%2zd p %5d %2d DB %7d, NB %7d No %5d Ds %19lld %19lld De %19lld %19lld Ns %19lld %19lld Ne %19lld %19lld\n",
                 i, xdd_data->tsh_tte[i].tte_thread_id, xdd_data->tsh_tte[i].tte_worker_thread_number, xdd_data->tsh_tte[i].tte_disk_xfer_size, xdd_data->tsh_tte[i].tte_net_xfer_size, xdd_data->tsh_tte[i].tte_net_xfer_calls,
                   xdd_data->tsh_tte[i].tte_disk_start,xdd_data->tsh_tte[i].tte_disk_start_k,
                   xdd_data->tsh_tte[i].tte_disk_end,xdd_data->tsh_tte[i].tte_disk_end_k,
                   xdd_data->tsh_tte[i].tte_net_start,xdd_data->tsh_tte[i].tte_net_start_k,
                   xdd_data->tsh_tte[i].tte_net_end,xdd_data->tsh_tte[i].tte_net_end_k);
           }
          fclose(fp);
          return (0);
}
