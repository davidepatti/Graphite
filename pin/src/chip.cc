#include "chip.h"

// definitions
using namespace std;

//TODO: c++-ize this, please oh please!
CAPI_return_t chipInit(int rank)
{
   
   THREADID pin_tid = PIN_ThreadId();

   pair<bool, UINT64> e = g_chip->core_map.find(pin_tid);


   //FIXME: Not sure what tid_map is, this should be enabled
   // if( g_chip->tid_map.find(rank)->first != false)
   // {
   //    ASSERT(false, "Error: Two cores tried to init the same rank!\n");
   // }

   if ( e.first == false ) {
      g_chip->tid_map[rank] = pin_tid;    
      g_chip->core_map.insert( pin_tid, rank );
      cerr << "chipInit initializing core: " << rank << endl;
   }   
   else
   {
      cerr << "chipInit Error initializing core twice: " << rank << endl;
//      ASSERT(false, "Error: Core tried to init more than once!\n");
   }

   return 0;
}

CAPI_return_t chipInitFreeRank(int *rank)
{
   
   THREADID pin_tid = PIN_ThreadId();

   pair<bool, UINT64> e = g_chip->core_map.find(pin_tid);


   //FIXME: Not sure what tid_map is, this should be enabled
   // if( g_chip->tid_map.find(rank)->first != false)
   // {
   //    ASSERT(false, "Error: Two cores tried to init the same rank!\n");
   // }

   if ( e.first == false ) {
      for(int i = 0; i < g_chip->num_modules - 1; i++)
      {
          if(g_chip->tid_map[i] == UINT_MAX)
          {
              g_chip->tid_map[i] = pin_tid;    
              g_chip->core_map.insert( pin_tid, i );
              *rank = i;
              cerr << "chipInit initializing core: " << i << endl;
              return 0;
          }
      }

      cerr << "chipInit Error: No Free Cores." << endl;
   }   
   else
   {
      cerr << "chipInit Error initializing core twice: " << rank << endl;
//      ASSERT(false, "Error: Core tried to init more than once!\n");
   }

   return 0;
}

CAPI_return_t chipRank(int *rank)
{
   THREADID pin_tid = PIN_ThreadId();

   pair<bool, UINT64> e = g_chip->core_map.find(pin_tid);

   *rank = (e.first == false) ? -1 : e.second;

   bool rank_ok = (*rank < g_chip->getNumModules());
   ASSERT(rank_ok, "Illegal rank value returned by chipRank!\n");

   return 0;
}

CAPI_return_t commRank(int *commid)
{
   int my_tid;
   chipRank(&my_tid);
   // FIXME: The network nodes should be moved up from within a core to
   //  directly within a chip.  When this happens, this will have to change.
   
   *commid = (my_tid < 0) ? -1 : g_chip->core[my_tid].coreCommID();

   return 0;
}

CAPI_return_t chipSendW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver, 
                        char *buffer, int size)
{
   int rank;
   chipRank(&rank);
   return g_chip->core[rank].coreSendW(sender, receiver, buffer, size);
}

CAPI_return_t chipRecvW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver, 
                        char *buffer, int size)
{
   int rank;
   chipRank(&rank);
   /*
   cout << "Starting receive on core " << rank << " (from:" << sender 
	<< ", to:" << receiver << ", ptr:" << hex << &buffer << dec
	<< ", size:" << size << ")" << endl;
   */ 
   return g_chip->core[rank].coreRecvW(sender, receiver, buffer, size);
}

// performance model wrappers

VOID perfModelRun(int rank, PerfModelIntervalStat *interval_stats)
{ 
   //int rank; 
   //chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   g_chip->core[rank].perfModelRun(interval_stats); 
}

VOID perfModelRun(int rank, PerfModelIntervalStat *interval_stats, 
                  REG *reads, UINT32 num_reads)
{ 
   //int rank;
   //chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   g_chip->core[rank].perfModelRun(interval_stats, reads, num_reads); 
}

VOID perfModelRun(int rank, PerfModelIntervalStat *interval_stats, bool dcache_load_hit, 
                  REG *writes, UINT32 num_writes)
{ 
   //int rank;
   //chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   g_chip->core[rank].perfModelRun(interval_stats, dcache_load_hit, writes, num_writes); 
}


PerfModelIntervalStat** perfModelAnalyzeInterval(const string& parent_routine, 
                                                 const INS& start_ins, const INS& end_ins)
{ 
   // using zero is a dirty hack 
   // assumes its safe to use core zero to generate perfmodels for all cores
   assert(g_chip->num_modules > 0);
   PerfModelIntervalStat* *array = new PerfModelIntervalStat*[g_chip->num_modules];

   for (INT32 i = 0; i < g_chip->num_modules; i++)
      array[i] = g_chip->core[0].perfModelAnalyzeInterval(parent_routine, start_ins, end_ins);
 
   return array; 
}

VOID perfModelLogICacheLoadAccess(int rank, PerfModelIntervalStat *stats, bool hit)
{ 
   //int rank;
   //chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   g_chip->core[rank].perfModelLogICacheLoadAccess(stats, hit); 
}
     
VOID perfModelLogDCacheStoreAccess(int rank, PerfModelIntervalStat *stats, bool hit)
{ 
    //int rank;
    //chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   g_chip->core[rank].perfModelLogDCacheStoreAccess(stats, hit); 
}

VOID perfModelLogBranchPrediction(int rank, PerfModelIntervalStat *stats, bool correct)
{ 
   //int rank;
   //chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   g_chip->core[rank].perfModelLogBranchPrediction(stats, correct); 
}


// organic cache model wrappers

bool icacheRunLoadModel(int rank, ADDRINT i_addr, UINT32 size)
{ 
   //int rank;
   //chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   return g_chip->core[rank].icacheRunLoadModel(i_addr, size); 
}

bool dcacheRunLoadModel(int rank, ADDRINT d_addr, UINT32 size)
{ 
   //int rank;
   //chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   return g_chip->core[rank].dcacheRunLoadModel(d_addr, size); 
}

bool dcacheRunStoreModel(int rank, ADDRINT d_addr, UINT32 size)
{ 
   //int rank;
   //chipRank(&rank);
   assert(0 <= rank && rank < g_chip->num_modules);
   return g_chip->core[rank].dcacheRunStoreModel(d_addr, size); 
}


// syscall model wrappers
void syscallEnterRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   int rank;
   chipRank(&rank);

   if(rank >= 0)
      g_chip->core[rank].getSyscallMdl()->runEnter(ctx, syscall_standard);
}

void syscallExitRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   int rank;
   chipRank(&rank);

   if(rank >= 0)
      g_chip->core[rank].getSyscallMdl()->runExit(ctx, syscall_standard);
}

// Sync client wrappers

void SimMutexInit(carbon_mutex_t *mux)
{
   int rank;
   chipRank(&rank);

   int commid;  
   commRank(&commid);
   assert( commid >= 0 );

   if(rank >= 0)
     g_chip->core[rank].getSyncClient()->mutexInit(commid, mux);
}

void SimMutexLock(carbon_mutex_t *mux)
{
   int rank;
   chipRank(&rank);

   int commid;  
   commRank(&commid);
   assert( commid >= 0 );

   if(rank >= 0)
     g_chip->core[rank].getSyncClient()->mutexLock(commid, mux);
}

void SimMutexUnlock(carbon_mutex_t *mux)
{
   int rank;
   chipRank(&rank);

   int commid;  
   commRank(&commid);
   assert( commid >= 0 );

   if(rank >= 0)
     g_chip->core[rank].getSyncClient()->mutexUnlock(commid, mux);
}

void SimCondInit(carbon_cond_t *cond)
{
   int rank;
   chipRank(&rank);

   int commid;  
   commRank(&commid);
   assert( commid >= 0 );

   if(rank >= 0)
     g_chip->core[rank].getSyncClient()->condInit(commid, cond);
}

void SimCondWait(carbon_cond_t *cond, carbon_mutex_t *mux)
{
   int rank;
   chipRank(&rank);

   int commid;  
   commRank(&commid);
   assert( commid >= 0 );

   if(rank >= 0)
     g_chip->core[rank].getSyncClient()->condWait(commid, cond, mux);
}

void SimCondSignal(carbon_cond_t *cond)
{
   int rank;
   chipRank(&rank);

   int commid;  
   commRank(&commid);
   assert( commid >= 0 );

   if(rank >= 0)
     g_chip->core[rank].getSyncClient()->condSignal(commid, cond);
}

void SimCondBroadcast(carbon_cond_t *cond)
{
   int rank;
   chipRank(&rank);

   int commid;  
   commRank(&commid);
   assert( commid >= 0 );

   if(rank >= 0)
     g_chip->core[rank].getSyncClient()->condBroadcast(commid, cond);
}

void SimBarrierInit(carbon_barrier_t *barrier, UINT32 count)
{
   int rank;
   chipRank(&rank);

   int commid;  
   commRank(&commid);
   assert( commid >= 0 );

   if(rank >= 0)
     g_chip->core[rank].getSyncClient()->barrierInit(commid, barrier, count);
}

void SimBarrierWait(carbon_barrier_t *barrier)
{
   int rank;
   chipRank(&rank);

   int commid;  
   commRank(&commid);
   assert( commid >= 0 );

   if(rank >= 0)
     g_chip->core[rank].getSyncClient()->barrierWait(commid, barrier);
}

// MCP wrappers
void MCPFinish()
{
   assert(g_MCP != NULL);
   g_MCP->finish();
}


void* MCPThreadFunc(void *dummy)
{
  // Declare local variables
  CAPI_return_t rtnVal;
  rtnVal = CAPI_Initialize(g_knob_total_cores);

   while( !g_MCP->finished() )
   {
      g_MCP->run();
      //usleep(1);
   }   
   cerr << "MCPThreadFunc - end!" << endl;
   return NULL;
//   pthread_exit(NULL);
}

// Chip class method definitions

Chip::Chip(int num_mods): num_modules(num_mods), core_map(3*num_mods), prev_rank(0) 
{
   tid_map = new THREADID [num_mods];
   core = new Core[num_mods];

   for(int i = 0; i < num_mods; i++) 
   {
      tid_map[i] = UINT_MAX;
      core[i].coreInit(this, i, num_mods);
   }

   InitLock(&maps_lock);
}

VOID Chip::fini(int code, VOID *v)
{
   ofstream out( g_knob_output_file.Value().c_str() );

   for(int i = 0; i < num_modules; i++) 
   {
      cout << "*** Core[" << i << "] summary ***" << endl;
      out << "*** Core[" << i << "] summary ***" << endl;
      core[i].fini(code, v, out); 
      cout << endl;
      out << endl;
   }

   out.close();
}


