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

/************************************************************************/
/*                                                                      */
/* EXTERNAL FUNCTIONS                                                   */
/*                                                                      */
/************************************************************************/

struct thread_pass_data_t
{
   thread_cb cb;
   void *arg;
};

//------------------------------------------------------------------------
static DWORD WINAPI ThreadEntryPointCommon( void *arg ) 
{
   thread_pass_data_t *pass_ptr = (thread_pass_data_t*)arg;

   pass_ptr->cb( pass_ptr->arg );
   delete pass_ptr;
   return 0;
}

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
   thread_handle_t th = ::CreateThread( nullptr,   // SECURITY OPTIONS
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
void GenerateGarbageWork( void *data ) 
{
   for (uint i = 0; i < 1000; ++i) {
      printf( "Garbage Thread: %i\n", i );
   }
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
void GenerateGarbage( char const *filename, size_t byte_count ) 
{
   thread_handle_t th = ThreadCreate( GenerateGarbageWork, nullptr );
   ThreadJoin( th );
}

#include <queue>
ThreadSafeQueue<std::string> gMessages;
bool gLoggerThreadRunning = true;

void LoggerThread( void* )
{
   FILE *fh = fopen( "log.log", "w+" );
   if (nullptr == fh) {
      return; 
   }

   std::string msg;
   while (gLoggerThreadRunning) {
      if (gMessages.pop(&msg)) {
         fprintf( fh, "%s\n", msg.c_str() );
      }
   }

   fclose(fh);
}

void LogPrint( char const *msg ) 
{
   gMessages.push( msg );
}

//------------------------------------------------------------------------
void ThreadDemo()
{
   // GenerateGarbage( "garbage.dat", 50 * 1024 * 1024 );


   thread_handle_t th = ThreadCreate( LoggerThread, nullptr );
   
   for (uint i = 0; i < 1000; ++i) {
      LogPrint( "Hello" );
      LogPrint( "Goodbye" );
      LogPrint( "Do we need more stuff?" );
   }

   ThreadJoin( th );

   for (uint i = 0; i < 10; ++i) {
      printf( "Main Thread: %i\n", i );
   }
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


