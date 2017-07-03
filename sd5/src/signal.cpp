/************************************************************************/
/*                                                                      */
/* INCLUDE                                                              */
/*                                                                      */
/************************************************************************/
#include "signal.h"

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
//------------------------------------------------------------------------
//
Signal::Signal()
{
   os_event = ::CreateEvent( nullptr, // security attributes, not needed
      TRUE,                           // Manual reset - do we manually reset, or auto reset after this is hit.
      FALSE,                          // initial state of the signal
      NULL );                         // name, used for cross-process communication
}

//------------------------------------------------------------------------
Signal::~Signal()
{
   ::CloseHandle( os_event );
   os_event = NULL;
   // nothing needed since we're destructing.
}

//------------------------------------------------------------------------
void Signal::signal_all()
{
   ::SetEvent( os_event );
}

//------------------------------------------------------------------------
void Signal::wait()
{
   DWORD result = ::WaitForSingleObject( os_event, INFINITE );
   if (result == WAIT_OBJECT_0) {
      // great, we succeeded!  [you can wait for multiple objects,
      // hence the WAIT_OBJECT_0 - meaning our the first object was
      // the one who caused us to activate.

      // this means everyone who is waiting is going to reset, but oh well - they already
      // got woken up. 
      ::ResetEvent(os_event);
   }
}


//------------------------------------------------------------------------
bool Signal::wait_for( uint ms ) 
{
   DWORD result = ::WaitForSingleObject( os_event, ms );
   if (result == WAIT_OBJECT_0) {
      // great, we succeeded!  [you can wait for multiple objects,
      // hence the WAIT_OBJECT_0 - meaning our the first object was
      // the one who caused us to activate.

      // this means everyone who is waiting is going to reset, but oh well - they already
      // got woken up. 
      ::ResetEvent(os_event);
      return true;
   }

   return false;
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------
#include "thread.h"

static bool gSignalTestRunning = false;

//------------------------------------------------------------------------
struct signal_test_t
{
   uint index;
   Signal *signal;
};

//------------------------------------------------------------------------
static void SignalTestThread( void *data ) 
{
   signal_test_t *test = (signal_test_t*)data;

   uint count = 0;
   while (gSignalTestRunning) {
      test->signal->wait();
      ++count;
      printf( "Thread[%i] was signaled [%i].\n", test->index, count );
   }

   delete test;
}

//------------------------------------------------------------------------
void SignalTest()
{
   uint num_threads = 8;
   Signal signal;

   gSignalTestRunning = true;

   for (uint i = 0; i < num_threads; ++i) {
      signal_test_t *test = new signal_test_t();
      test->index = i;
      test->signal = &signal;
      
      // thread will handle deleting the variable.
      thread_handle_t handle = ThreadCreate( SignalTestThread, test );
      ThreadDetach( handle );
   }

   // give all the threads a chance to startup.
   // not guaranteed, but good enough for a test.
   ThreadSleep( 50 );

   // now, signal them 50 times - do all go at once, or 
   // do they go one at a time?
   for (uint i = 0; i < 50; ++i) {
      signal.signal_all();
      ThreadSleep(100);
   }

   gSignalTestRunning = false;
   signal.signal_all();

   // not the safest - but just wait till everyone has exited.
   ThreadSleep(100);
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



