#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
//#include <xdd.h>
//
#ifndef MAX
#define MAX(a,b) (a > b ? a : b)
#endif
#ifndef MIN
#define MIN(a,b) (a < b ? a : b)
#endif

  int32_t target_pid;
  int32_t nops_pid, nops_op;
  int64_t size_enter, size_exit, sum_size_op, total_size_op;
  double rsecs_beg_op, rsecs_end_op;
//
//
/**
 *  Parse and interpret lines
 */
void parse_line (
                 char *cmdline 
                )
{
  static char *seps = " ,;\n\t{}";
  char *keyword;
  int64_t secs, nsecs;
  double rsecs;
 
  char string[256];

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
        fprintf(stderr,"found enter\n");
    keyword = strtok(NULL,seps);
    if ( !strncmp(keyword,"recvfrom",8) ) 
    {
        fprintf(stderr,"found recvfrom\n");
    while ( (keyword = strtok(NULL,seps)) != NULL)
    {
      if ( !strncmp(keyword,"pid" ,  3 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        if ( atoi(keyword) != target_pid) return;
        nops_op++;
        fprintf(stderr,"matched target_pid= %d\n",target_pid);
      }
      if ( !strncmp(keyword,"secs" , 4 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        secs = atoi(keyword);
        fprintf(stderr,"matched secs= %ld\n",secs);
      }
      if ( !strncmp(keyword,"nsecs" , 5 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        nsecs = atoi(keyword);
        rsecs = (double)secs + (double)nsecs/1.e9;
        if (nops_op == 1 ) rsecs_beg_op = rsecs;
        
        fprintf(stderr,"matched nsecs= %ld, %15.9f\n",nsecs,rsecs);
      }
      if ( !strncmp(keyword,"size" , 4 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        size_enter = atoi(keyword);
        if (nops_op == 1 ) total_size_op = size_enter;
        fprintf(stderr,"matched enter bytes= %ld\n",size_enter);
      }
    }
  }
 }
/* exit */
  else
  if ( !strncmp(keyword,"exit",5) ) 
  {
        fprintf(stderr,"found exit\n");
    keyword = strtok(NULL,seps);
    if ( !strncmp(keyword,"recvfrom",8) ) 
    {
        fprintf(stderr,"found recvfrom\n");
    while ( (keyword = strtok(NULL,seps)) != NULL)
    {
      if ( !strncmp(keyword,"pid" ,  3 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        if ( atoi(keyword) != target_pid) return;
        fprintf(stderr,"matched target_pid= %d\n",target_pid);
      }
      if ( !strncmp(keyword,"secs" , 4 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        secs = atoi(keyword);
        fprintf(stderr,"matched secs= %ld\n",secs);
      }
      if ( !strncmp(keyword,"nsecs" , 5 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        nsecs = atoi(keyword);
        rsecs_end_op = (double)secs + (double)nsecs/1.e9;
        fprintf(stderr,"matched nsecs= %ld, %15.9f\n",nsecs,rsecs_end_op);
      }
      if ( !strncmp(keyword,"ret" , 3 ) ) 
      {
        if ( (keyword = strtok(NULL,seps)) == NULL ) return;
        size_exit = atoi(keyword);
        sum_size_op += size_exit;
        fprintf(stderr,"matched enter bytes= %ld\n",size_enter);
        if (size_exit == size_enter)
        {
          if ( sum_size_op != total_size_op ) 
            fprintf(stderr,"sum=%ld, size=%ld DONT match\n",sum_size_op,total_size_op);
          nops_pid++;
          fprintf(stderr,"end-of-op size %ld time %15.9f ops %ld ops_pid %ld \n",
             total_size_op, rsecs_end_op-rsecs_beg_op, nops_op, nops_pid);
          nops_op = 0;
          sum_size_op = 0;

        }
      }
    }
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
 *  Dispatch the next file line, if any \n
 *
 */
int  parse_file (
                   FILE *file
                  ) 
{
  char 
     line[256];

  int 
    nlines = 0;

  target_pid = 7523;
  sum_size_op = 0;
  while (fgets(line,256,file) != NULL)
  {
    chop(line);
    parse_line(line);
  }

  return (0);
}

       int
       main(int argc, char *argv[])
       {
           char *token;
           char *delims = " {}";
           int j, j_enter, j_op, iline;
           int pid, mypid;
           FILE *fp;
           char line[256];

  /* --------------------- */
  /* Executable Statements */
  /* --------------------- */

           if (argc != 4) {
               fprintf(stderr, "Usage: %s filespec pid operation\n",
                       argv[0]);
               exit(EXIT_FAILURE);
           }
            fp = fopen( argv[1], "r");
            if ( fp == NULL )
            {
              fprintf(stderr,"error: can't open file: %s\n",argv[1]);
              exit -1;
            }
              fprintf(stderr,"opened %s\n", argv[1]);
              parse_file(fp);
            return (0);
  }
