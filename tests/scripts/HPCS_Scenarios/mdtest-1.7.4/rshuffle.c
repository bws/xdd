/* fileread.c written by detour@metalshell.com
 *
 * example of reading a file with fopen() and fgets()
 * 
 * http://www.metalshell.com/
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
 
void stripnl(char *str) {
  while(strlen(str) && ( (str[strlen(str) - 1] == 13) || 
       ( str[strlen(str) - 1] == 10 ))) {
    str[strlen(str) - 1] = 0;
  }
}
 
int main(int argc, char **argv) {
  int MAX_LINE_LEN = 32;
  FILE *infile;
  char **lines;
  char *linetmp;
  char fname[256];
  int lcount, c, i, j;
  int maxlines = 100000; /* default */
 
  /* Read in the filename */
   c = getopt(argc, argv, "f:");
   if (c == -1) {
      printf("Usage: %s -f file_name -n max#lines\n", argv[0]);
      exit (1);
   }
   switch (c) {
      case 'f':
              strncpy(fname, optarg, MAX_LINE_LEN);  break;
      case 'n':
              maxlines = atoi(optarg);    break;
      default: 
         printf("Usage: %s -f file_name -n max#lines\n", argv[0]);
         exit (1);
   }

 
  /* We need to get rid of the newline char. */
  stripnl(fname);
 
  /* Open the file.  If NULL is returned there was an error */
  if((infile = fopen(fname, "r")) == NULL) {
    printf("Error Opening File.\n");
    exit(1);
  }
  
  lines = (char **)malloc(maxlines * sizeof(char *));
        lcount = 0;
  lines[lcount]  = (char *)malloc(MAX_LINE_LEN * sizeof(char));
  while( fgets(&lines[lcount][0], MAX_LINE_LEN, infile) != NULL ) {
    /* Get each line from the infile */
    lcount++;
    /* allocate the next one */
    lines[lcount]  = (char *)malloc(MAX_LINE_LEN * sizeof(char));
  }
 
  fclose(infile);  /* Close the file */

  /* Random shuffle by Sattolo */ 
  linetmp = (char *)malloc(MAX_LINE_LEN * sizeof(char));
  i = lcount;
  while ( i > 1) {
  i = i - 1;
  j = i*random()/RAND_MAX;
  strncpy(linetmp     , &lines[i][0] ,MAX_LINE_LEN); /* tmp=i */
  strncpy(&lines[i][0], &lines[j][0] ,MAX_LINE_LEN); /* i=j   */
  strncpy(&lines[j][0],  linetmp     ,MAX_LINE_LEN); /* j=tmp */
}
  /* print to stdout */
  for (i=0; i< lcount; i++ ) {
   printf("%s",&lines[i][0]);
  }


}
/*
A sample implementation of Sattolo's algorithm in Python is:

from random import randrange
 
def sattoloCycle(items):
    i = len(items)
    while i > 1:
        i = i - 1
        j = randrange(i)  # 0 <= j <= i-1
        items[j], items[i] = items[i], items[j]
    return
*/
