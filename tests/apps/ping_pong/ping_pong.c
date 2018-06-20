#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "carbon_user.h"

void* ping_pong(void *threadid);

int global1 = 0;
//int global2 = 0;

int main(int argc, char* argv[])  // main begins
{
   CarbonStartSim(argc, argv);

   int num_threads = 2;
   carbon_thread_t threads[num_threads];

   for(unsigned int i = 0; i < num_threads; i++)
   {
       printf("Spawning thread: %d\n", i);
       threads[i] = CarbonSpawnThread(ping_pong, (void *) i);
   }

   for(unsigned int i = 0; i < num_threads; i++)
       CarbonJoinThread(threads[i]);

   CarbonStopSim();
   return 0;
} // main ends


void* ping_pong(void *threadid)
{
    CAPI_Initialize((int)threadid);

    if ((int)threadid==0) global1++;
//    if ((int)threadid==1) global2++;

    if ((int)threadid==1) printf("global var has value: %d\n", global1);

   // if (global1==1) printf("Reading global1\n");
   // if (global2==1) printf("Reading global2\n");

//    if (global1==1) printf("I'm thread %d and I see global1 = %d !\n",tid,global1);
//    if (global2==1) printf("I'm thread %d and I see global2 = %d !\n",tid,global2);

   return NULL;
}

