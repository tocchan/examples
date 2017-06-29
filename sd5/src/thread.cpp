/************************************************************************/
/*                                                                      */
/* INCLUDE                                                              */
/*                                                                      */
/************************************************************************/
#include "thread.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/************************************************************************/
/*                                                                      */
/* DEFINES AND CONSTANTS                                                */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* MACROS                                                               */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* TYPES                                                                */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* STRUCTS                                                              */
/*                                                                      */
/************************************************************************/
//------------------------------------------------------------------------
// Struct used to pass a name for the attached debugger
#define MS_VC_EXCEPTION      (0x406d1388)

#pragma pack(push, 8)
   struct THREADNAME_INFO
   {
      DWORD type;            // must be 0x1000
      const char *name;      // name
      DWORD thread_id;      // -1 for calling thread
      DWORD flags;         // must be 0, reserved for future use
   };
#pragma pack(pop)

//------------------------------------------------------------------------
struct thread_pass_data_t
{
   thread_cb cb;
   void *arg;
};

/************************************************************************/
/*                                                                      */
/* CLASSES                                                              */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* LOCAL VARIABLES                                                      */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* GLOBAL VARIABLES                                                     */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* LOCAL FUNCTIONS                                                      */
/*                                                                      */
/************************************************************************/

//------------------------------------------------------------------------
static DWORD WINAPI ThreadEntryPointCommon( void *arg ) 
{
   thread_pass_data_t *pass_ptr = (thread_pass_data_t*)arg;

   pass_ptr->cb( pass_ptr->arg );
   delete pass_ptr;
   return 0;
}

/************************************************************************/
/*                                                                      */
/* EXTERNAL FUNCTIONS                                                   */
/*                                                                      */
/************************************************************************/

//------------------------------------------------------------------------
// Creates a thread with the entry point of cb, passed data
thread_handle_t ThreadCreate( thread_cb cb, void *data )
{
   // handle is like pointer, or reference to a thread
   // thread_id is unique identifier
   thread_pass_data_t *pass = new thread_pass_data_t();
   pass->cb = cb;
   pass->arg = data;

   DWORD thread_id;
   thread_handle_t th = (thread_handle_t) ::CreateThread( nullptr,   // SECURITY OPTIONS
      0,                         // STACK SIZE, 0 is default
      ThreadEntryPointCommon,    // "main" for this thread
      pass,                     // data to pass to it
      0,                         // initial flags
      &thread_id );              // thread_id

   return th;
}

//------------------------------------------------------------------------
void ThreadSleep( uint ms )
{
   ::Sleep( ms );
}

//------------------------------------------------------------------------
void ThreadYield()
{
   ::SwitchToThread();
}

//------------------------------------------------------------------------
// Releases my hold on this thread.
void ThreadDetach( thread_handle_t th )
{
   ::CloseHandle( th );
}

//------------------------------------------------------------------------
void ThreadJoin( thread_handle_t th ) 
{
   ::WaitForSingleObject( th, INFINITE );
   ::CloseHandle( th );
}

//------------------------------------------------------------------------
void ThreadJoin( thread_handle_t *th, uint count ) 
{
   for (uint i = 0; i < count; ++i) {
      ThreadJoin(th[i]);
   }
}

//------------------------------------------------------------------------
thread_id_t ThreadGetCurrentID()
{
   return (thread_id_t) (uintptr_t) ::GetCurrentThreadId();
}

//------------------------------------------------------------------------
void ThreadSetNameInVisualStudio( char const *name )
{
     if (nullptr == name) {
      return;
   }

   thread_id_t id = ThreadGetCurrentID();
   if (0 != id) {
      THREADNAME_INFO info;
      info.type = 0x1000;
      info.name = name;
      info.thread_id =  (DWORD)(uintptr_t)id;
      info.flags = 0;

      __try 
      {
         RaiseException( MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)(&info) );
      } 
      __except (EXCEPTION_CONTINUE_EXECUTION) 
      {
      }
   }
}



//------------------------------------------------------------------------
// EXAMPLE CODE : LOGGER
//------------------------------------------------------------------------
#include "ts_queue.h"
#include "signal.h"
#include "event.h"


ThreadSafeQueue<std::string> gMessages;

thread_handle_t gLoggerThread = nullptr;
bool gLoggerThreadRunning = true;
Signal gLogSignal; 
Event<std::string*> gLogEvent; 

//------------------------------------------------------------------------
uint FlushMessages( FILE *fh )
{
   uint count = 0;
   std::string msg;

   while (gMessages.dequeue(&msg)) {
      gLogEvent.trigger( &msg );
      ++count;
   }

   return count;
}

//------------------------------------------------------------------------
void LogWriteToFile( void *user_arg, std::string *msg ) 
{
   FILE *fh = (FILE*) user_arg;
   fprintf( fh, "%s\n", msg->c_str() ); 
}

//------------------------------------------------------------------------
void LogWriteToDebugger( void*, std::string *msg )
{
   OutputDebugStringA( msg->c_str() );
   OutputDebugStringA( "\n" );
}


//------------------------------------------------------------------------
void LoggerThread( void* )
{
   FILE *fh = nullptr;
   errno_t err = fopen_s( &fh, "log.log", "w+" );
   if ((err != 0) || (fh == nullptr)) {
      return; 
   }

   gLogEvent.subscribe( fh, LogWriteToFile );

   while (gLoggerThreadRunning) {
      gLogSignal.wait();
      FlushMessages(fh);
   }

   FlushMessages(fh);
   fclose(fh);
}

//------------------------------------------------------------------------
void LogPrint( char const *msg ) 
{
   gMessages.enqueue( msg );
   gLogSignal.signal_all();
}

//------------------------------------------------------------------------
void LogStartup()
{
   gLoggerThreadRunning = true;
   gLoggerThread = ThreadCreate( LoggerThread, nullptr );
}

//------------------------------------------------------------------------
void LogShutdown()
{
   gLoggerThreadRunning = false;
   gLogSignal.signal_all();
   ThreadJoin( gLoggerThread );
   gLoggerThread = INVALID_THREAD_HANDLE;

}


//------------------------------------------------------------------------
void ThreadDemo()
{
   // GenerateGarbage( "garbage.dat", 50 * 1024 * 1024 );
   LogStartup();

   gLogEvent.subscribe( nullptr, LogWriteToDebugger );
   
   for (uint i = 0; i < 1000; ++i) {
      LogPrint( "Main Thread!" );
      ThreadSleep(10);
   }

   LogShutdown();
}

/************************************************************************/
/*                                                                      */
/* COMMANDS                                                             */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* UNIT TESTS                                                           */
/*                                                                      */
/************************************************************************/


