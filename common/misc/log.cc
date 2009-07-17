#include <sys/time.h>
#include <sys/syscall.h>
#include <stdarg.h>
#include <string.h>

#include "log.h"
#include "config.h"
#include "simulator.h"
#include "core_manager.h"

using namespace std;

Log *Log::_singleton;

const UInt32 Log::MODULE_LENGTH;

static string formatFileName(const char* s)
{
   return Sim()->getConfig()->formatOutputFileName(s);
}

Log::Log(Config &config)
   : _coreCount(config.getTotalCores())
   , _startTime(0)
{
   assert(Config::getSingleton()->getProcessCount() != 0);

   _coreFiles = new FILE* [_coreCount];
   _simFiles = new FILE* [_coreCount];

   for (core_id_t i = 0; i < _coreCount; i++)
   {
      _coreFiles[i] = NULL;
      _simFiles[i] = NULL;
   }

   _coreLocks = new Lock [_coreCount];
   _simLocks = new Lock [_coreCount];

   _systemFile = NULL;

   _defaultFile = fopen(formatFileName("system-default.log").c_str(),"w");

   std::set<std::string> disabledModulesUnformatted;
   Config::getSingleton()->getDisabledLogModules(disabledModulesUnformatted);
   for (std::set<std::string>::iterator it = disabledModulesUnformatted.begin();
        it != disabledModulesUnformatted.end();
        it++)
   {
      string formatted;

      for (unsigned int i = 0; i < min(MODULE_LENGTH, it->length()); i++)
      {
         formatted.push_back((*it)[i]);
      }

      for (unsigned int i = formatted.length(); i < MODULE_LENGTH; i++)
      {
         formatted.push_back(' ');
      }

      assert(formatted.length() == MODULE_LENGTH);
      _disabledModules.insert(formatted);
   }

   _loggingEnabled = Config::getSingleton()->getLoggingEnabled();

   assert(_singleton == NULL);
   _singleton = this;
}

Log::~Log()
{
   _singleton = NULL;

   for (core_id_t i = 0; i < _coreCount; i++)
   {
      if (_coreFiles[i])
         fclose(_coreFiles[i]);
      if (_simFiles[i])
         fclose(_simFiles[i]);
   }

   delete [] _coreLocks;
   delete [] _simLocks;
   delete [] _coreFiles;
   delete [] _simFiles;

   if (_systemFile)
      fclose(_systemFile);
   fclose(_defaultFile);
}

Log* Log::getSingleton()
{
   assert(_singleton);
   return _singleton;
}

Boolean Log::isEnabled(const char* module)
{
   return _disabledModules.find(module) == _disabledModules.end();
}

Boolean Log::isLoggingEnabled()
{
   return _loggingEnabled;
}

UInt64 Log::getTimestamp()
{
   timeval t;
   gettimeofday(&t, NULL);
   UInt64 time = (((UInt64)t.tv_sec) * 1000000 + t.tv_usec);
   if (_startTime == 0) _startTime = time;
   return time - _startTime;
}

void Log::discoverCore(core_id_t *core_id, bool *sim_thread)
{
   CoreManager *core_manager;

   if (!Sim() || !(core_manager = Sim()->getCoreManager()))
   {
      *core_id = INVALID_CORE_ID;
      *sim_thread = false;
      return;
   }

   *core_id = core_manager->getCurrentCoreID();
   if (*core_id != INVALID_CORE_ID)
   {
      *sim_thread = false;
      return;
   }
   else
   {
      *core_id = core_manager->getCurrentSimThreadCoreID();
      *sim_thread = true;
      return;
   }
}

void Log::getFile(core_id_t core_id, bool sim_thread, FILE **file, Lock **lock)
{
   // we use on-demand file allocation to prevent contention between
   // processes for files

   *file = NULL;
   *lock = NULL;

   if (core_id == INVALID_CORE_ID)
   {
      // System file -- use process num if available
      UInt32 procNum = Config::getSingleton()->getCurrentProcessNum();
      
      if (procNum != (UInt32)-1)
      {
         if (_systemFile == NULL)
         {
            assert(procNum < Config::getSingleton()->getProcessCount());
            char filename[256];
            sprintf(filename, "system_%u.log", procNum);
            _systemFile = fopen(formatFileName(filename).c_str(), "w");
            assert(_systemFile != NULL);
         }

         *file = _systemFile;
         *lock = &_systemLock;
      }
      else
      {
         *file = _defaultFile;
         *lock = &_defaultLock;
      }
   }
   else if (sim_thread)
   {
      // sim thread file
      if (_simFiles[core_id] == NULL)
      {
         assert(core_id < _coreCount);
         char filename[256];
         sprintf(filename, "sim_%u.log", core_id);
         _simFiles[core_id] = fopen(formatFileName(filename).c_str(), "w");
         assert(_simFiles[core_id] != NULL);
      }

      *file = _simFiles[core_id];
      *lock = &_simLocks[core_id];
   }
   else
   {
      // core file
      if (_coreFiles[core_id] == NULL)
      {
         assert(core_id < _coreCount);
         char filename[256];
         sprintf(filename, "app_%u.log", core_id);
         _coreFiles[core_id] = fopen(formatFileName(filename).c_str(), "w");
         assert(_coreFiles[core_id] != NULL);
      }

      // Core file
      *file = _coreFiles[core_id];
      *lock = &_coreLocks[core_id];
   }
}

std::string Log::getModule(const char *filename)
{
#ifdef LOCK_LOGS
   ScopedLock sl(_modules_lock);
#endif
   std::map<const char*, std::string>::const_iterator it = _modules.find(filename);

   if (it != _modules.end())
   {
      return it->second;
   }
   else
   {
      // build module string
      string mod;

      // find actual file name ...
      const char *ptr = strrchr(filename, '/');
      if (ptr != NULL)
         filename = ptr + 1;

      for (UInt32 i = 0; i < MODULE_LENGTH && filename[i] != '\0'; i++)
         mod.push_back(filename[i]);

      while (mod.length() < MODULE_LENGTH)
         mod.push_back(' ');

      pair<const char*, std::string> p(filename, mod);
      _modules.insert(p);

      return mod;
   }
}

void Log::log(ErrorState err, const char* source_file, SInt32 source_line, const char *format, ...)
{
   if (!_loggingEnabled && err != Error)
      return;

   // Called in LOG_PRINT macro (see log.h)
//   if (!isEnabled(source_file))
//      return;

   core_id_t core_id;
   bool sim_thread;
   discoverCore(&core_id, &sim_thread);
   
   FILE *file;
   Lock *lock;

   getFile(core_id, sim_thread, &file, &lock);
   int tid = syscall(__NR_gettid);


   char message[512];
   char *p = message;

   // This is ugly, but it just prints the time stamp, process number, core number, source file/line
   if (core_id != INVALID_CORE_ID) // valid core id
      p += sprintf(p, "%-10llu [%5d]  (%2i) [%2i]%s[%s:%4d]  ", getTimestamp(), tid, Config::getSingleton()->getCurrentProcessNum(), core_id, (sim_thread ? "* " : "  "), source_file, source_line);
   else if (Config::getSingleton()->getCurrentProcessNum() != (UInt32)-1) // valid proc id
      p += sprintf(p, "%-10llu [%5d]  (%2i) [  ]  [%s:%4d]  ", getTimestamp(), tid, Config::getSingleton()->getCurrentProcessNum(), source_file, source_line);
   else // who knows
      p += sprintf(p, "%-10llu [%5d]  (  ) [  ]  [%s:%4d]  ", getTimestamp(), tid, source_file, source_line);

   switch (err)
   {
   case None:
   default:
      break;

   case Warning:
      p += sprintf(p, "*WARNING* ");
      break;

   case Error:
      p += sprintf(p, "*ERROR* ");
      break;
   };

   va_list args;
   va_start(args, format);
   p += vsprintf(p, format, args);
   va_end(args);

   p += sprintf(p, "\n");

   lock->acquire();

   fputs(message, file);
   fflush(file);

   lock->release();

   switch (err)
   {
   case Error:
      fputs(message, stderr);
      abort();
      break;

   case Warning:
      fputs(message, stderr);
      break;

   case None:
   default:
      break;
   }
}
