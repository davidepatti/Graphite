// config.h
//
// The Config class is used to set, store, and retrieve all the configurable
// parameters of the simulator.
//
// Initial creation: Sep 18, 2008 by jasonm

#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <set>
#include <string>
#include <iostream>
#include <cassert>
#include "fixed_types.h"

struct NetworkModelAnalyticalParameters;

class Config {
 public:
   typedef std::vector<UInt32> CoreToProcMap;
   typedef std::vector<UInt32> CoreList;
   typedef std::vector<UInt32>::const_iterator CLCI;
 private:
   void GenerateCoreMap();

   UInt32  num_process;          // Total number of processes (incl myself)
   UInt32  total_cores;          // Total number of cores in all processes

   UInt32  my_proc_num;          // Process number for this process

   // This data structure keeps track of which cores are in each process.
   // It is an array of size num_process where each element is a list of
   // core numbers.  Each list specifies which cores are in the corresponding
   // process.
   CoreToProcMap core_to_proc_map;
   CoreList* proc_to_core_list_map;

   UInt32  MCP_process;          // The process where the MCP lives

   NetworkModelAnalyticalParameters *analytic_network_parms;
   
 public:
   Config();
   ~Config();

   void loadFromFile(char* filename);
   void loadFromCmdLine();

   // Return the number of processes involved in this simulation
   UInt32 numProcs() { return num_process; }
   void setNumProcs(UInt32 in_num_process) { num_process = in_num_process; }

   // Retrieve and set the process number for this process (I'm expecting
   //  that the initialization routine of the Transport layer will set this)
   UInt32 myProcNum() { return my_proc_num; }
   void setProcNum(UInt32 in_my_proc_num) { my_proc_num = in_my_proc_num; }

   // Return the number of the process that should contain the MCP
   //UInt32 MCPProcNum() { return MCP_process; }
   UInt32 MCPCoreNum() { return totalCores() - 1; }

   // Return the number of modules (cores) in a given process
   UInt32 numCoresInProcess(UInt32 proc_num)
      { assert(proc_num < num_process); return proc_to_core_list_map[proc_num].size(); }

   UInt32 numLocalCores() { return numCoresInProcess(myProcNum()); }

   // Return the total number of modules in all processes
   UInt32 totalCores() { return total_cores; }

   // Return an array of core numbers for a given process
   //  The returned array will have numMods(proc_num) elements
   const CoreList getCoreListForProcess(UInt32 proc_num)
      { assert(proc_num < num_process); return proc_to_core_list_map[proc_num]; }

   UInt32 procNumForCore(UInt32 core)
      { assert(core < total_cores); return core_to_proc_map[core]; }

   const NetworkModelAnalyticalParameters *getAnalyticNetworkParms() const
      { return analytic_network_parms; }

   // Fills in an array with the models for each static network
   void getNetworkModels(UInt32 *) const;

   // Knobs
   Boolean isSimulatingSharedMemory() const;
   Boolean isPerfModeling() const;   

   // Logging
   void getDisabledLogModules(std::set<std::string> &mods) const;
   const char* getOutputFileName() const;
};

extern Config *g_config;

#endif
