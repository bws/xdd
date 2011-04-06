#define _DO_MPI

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef _DO_MPI
#include <mpi.h>
#endif

void main(int argc, char **argv)
{
   int rank, size, status;
   char *str, *token, *saveptr;
   char delim[8]={":"};
   char kmd[256]={0};
   char crank[16]={"0"};
   char cfrst[2]={0};
   char clast[2]={0};
   int i, j;
   time_t ts;
   
#ifdef _DO_MPI
   MPI_Init(&argc,&argv);
   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   MPI_Comm_size(MPI_COMM_WORLD, &size);
#endif

   sprintf(crank,"%d",rank);
     
/* replace ":" with rank in every argument argument */
     for (i=1;i<argc;i++)
     {
       str = argv[i];
       strncpy(cfrst,str,1);
       strncpy(clast,str+strlen(str)-1,1);
       for (j = 1, saveptr = NULL; ; j++, str = NULL)
       {
          token = strtok_r(str, delim, &saveptr);
          if (token == NULL)
                 break;
          if (j == 1)
          {
            if (!strncmp(cfrst,delim,1))
              strcat(kmd,crank);
              strcat(kmd,token);
           }
           else
           {
              strcat(kmd,crank);
              strcat(kmd,token);
            }
        }
        if (!strncmp(clast,delim,1)) strcat(kmd,crank);
                                     strcat(kmd," ");
     }
   ts = time(NULL);
   printf("RANK [%04d] launched %s --- %s",rank,kmd,ctime(&ts));
/* execv(argv[1],&argv[1]); */
   system(kmd);
/* wait(&status); */

   ts = time(NULL);
   printf("RANK [%04d] at MPI_Barrier %s --- %s",rank,kmd,ctime(&ts));

#ifdef _DO_MPI
   MPI_Barrier(MPI_COMM_WORLD);
   MPI_Finalize();
#endif

}

