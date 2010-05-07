/* rshuffled.c written by hodsonsw@ornl.gov
 *
 * get all files in list of directories and randomly shuffle to stdout
 * 
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
 
int main(int argc, char **argv) {
  char **lines = NULL;
  char *linetmp;
  int lcount, i, j;
  DIR *dir;
  struct dirent *entry;
  size_t slen;
 
  /* Parse command line for directories, process each dir entry */
   lcount = 0;
   i = 1;
   while (i<argc) { 
     if ((dir = opendir(argv[i])) == NULL)
       perror("opendir() error");
     else {
       while ((entry = readdir(dir)) != NULL) {
         if (strncmp(entry->d_name,".",1)) {
           slen = 2 + strlen(argv[i]) + 1 + strlen(entry->d_name) + 1;
           lines          = (char **)realloc(lines, (lcount+1) * sizeof(char *));
           lines[lcount]  = (char  *) malloc(slen * sizeof(char  ));
           strcpy(lines[lcount],"./");
           strcat(lines[lcount],argv[i]);
           strcat(lines[lcount],"/");
           strcat(lines[lcount],entry->d_name);
           strcat(lines[lcount],"\0");
 /*        printf("argv=%s entry: %s   slen=%d, lines[%d]=%s\n",argv[i],entry->d_name, slen, lcount, lines[lcount]); */
                        lcount++;
         }
       }
       closedir(dir);
     }
     i++;
   }
  /* Random shuffle by Sattolo */ 
  linetmp = (char *)malloc(NAME_MAX * sizeof(char));
  i = lcount;
  while ( i > 1) {
  i = i - 1;
  j = i*random()/RAND_MAX;
  if (strlen(lines[i]) < strlen(lines[j])) lines[i] = realloc(lines[i], strlen(lines[j]) * sizeof(char  ));
  if (strlen(lines[i]) > strlen(lines[j])) lines[j] = realloc(lines[j], strlen(lines[i]) * sizeof(char  ));
  strcpy(linetmp, lines[i]); /* tmp=i */
  strcpy(lines[i], lines[j]); /* i=j   */
  strcpy(lines[j],  linetmp); /* j=tmp */
}
  /* print to stdout */
  for (i=0; i< lcount; i++ ) {
   printf("%s\n",lines[i]);
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
