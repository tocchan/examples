#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <conio.h>
#include <stdio.h>

#include "src/common.h"
#include "src/time.h"
#include "src/memory.h"
#include "src/memory_demo.h"
#include "src/thread.h"
#include "src/signal.h"

#include "src/profile.h"


// Recursive Template Example with MAX
template <typename T>
T Max( T const &a, T const &b ) 
{
   return a;
}

template <typename T, typename ...ARGS>
T Max( T const &a, T const &b, ARGS ...args ) 
{
   return Max( a, Max( b, args... ) );
}

// Example thread with parameters
void ThreadDoWork( int a, float b, char const *c ) 
{
   printf( "%i, %.4f, %s", a, b, c );
}


typedef void* (*alloc_cb)( size_t );
typedef void (*free_cb)( void* );

#include <vector>

#define ALLOC_SIZE (1024)
uint const TOTAL_ALLOC_COUNT = 10000;
uint const LIVE_ALLOC_COUNT = 1000;
uint const CYCLE_ALLOC_COUNT = TOTAL_ALLOC_COUNT - LIVE_ALLOC_COUNT;

// So this will be our baseline.  Fairly standard alloc and frees
// and frees happen in the order they were allocated [similar to our use case
// for the profiler]
void BaseLine()
{
   // 8 KB of data on the stack, 1 MB of data on the heap [1000 * 1024]
   void* allocations[LIVE_ALLOC_COUNT];

   // do initial allocations;
   for (uint i = 0; i < LIVE_ALLOC_COUNT; ++i) {
      allocations[i] = malloc(ALLOC_SIZE);
   }

   // now free and alloc in order for the remaining 
   // [we're moving through this buffer in a circle - like a ring]
   for (uint i = 0; i < CYCLE_ALLOC_COUNT; ++i) {
      uint idx = i % LIVE_ALLOC_COUNT;
      free( allocations[idx] );
      allocations[idx] = malloc( ALLOC_SIZE );
   }

   // Now do the frees;
   for (uint i = 0; i < LIVE_ALLOC_COUNT; ++i) {
      free( allocations[i] );
   }
}

//--------------------------------------------------------------------
// exact same code as above, but I'm using passed in function pointers
void FunctionPointerTest( alloc_cb my_alloc, free_cb my_free ) 
{
   // 8 KB of data on the stack, 1 MB of data on the heap [1000 * 1024]
   void* allocations[LIVE_ALLOC_COUNT];

   // do initial allocations;
   for (uint i = 0; i < LIVE_ALLOC_COUNT; ++i) {
      allocations[i] = my_alloc(ALLOC_SIZE);
   }

   // now free and alloc in order for the remaining 
   // [we're moving through this buffer in a circle - like a ring]
   for (uint i = 0; i < CYCLE_ALLOC_COUNT; ++i) {
      uint idx = i % LIVE_ALLOC_COUNT;
      my_free( allocations[idx] );
      allocations[idx] = my_alloc( ALLOC_SIZE );
   }

   // Now do the frees;
   for (uint i = 0; i < LIVE_ALLOC_COUNT; ++i) {
      my_free( allocations[i] );
   }
}

//--------------------------------------------------------------------
// Baseline for the function pointer version.
uint gAllocCount = 0;
void* TestAlloc( size_t size ) 
{
   return malloc(size);
}

void TestFree( void *ptr ) 
{
   free( ptr );
}

//--------------------------------------------------------------------
int main( int argc, char const *argv[] ) 
{   
   // run multiple tests - fluctuations in the machine change the result
   // so we want an average one. 
   uint const NUM_TESTS = 10;

   // First, let's test our baseline.
   for (uint i = 0; i < NUM_TESTS; ++i) {
      PROFILE_LOG_SCOPE("BaseLine");
      BaseLine();
   }
   pause();

   // Next, lets see how much adding a "virtual function" adds to this
   // [people say virtual allocs are slow - so.. how slow?]
   for (uint i = 0; i < NUM_TESTS; ++i) {
      PROFILE_LOG_SCOPE("FunctionPointerTest");
      FunctionPointerTest( TestAlloc, TestFree );
   }
   pause();
   // should have resulted in nearly identical results;
   


   pause();
   return 0;
}