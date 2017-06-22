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

#include <random>

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


__forceinline 
uint CompareAndSet( uint volatile *ptr, uint const comparand, uint const value )
{
   /*
   uint const old_value = *ptr;
   if (old_value == comparand) {
      *ptr = value;
   }
   return old_value;
   */

   return ::InterlockedCompareExchange( ptr, value, comparand );
}

template <typename T>
__forceinline T* CompareAndSet( T *volatile *ptr, T *comparand, T *value ) 
{
   return (T*)::InterlockedCompareExchangePointerNoFence( (PVOID volatile*)ptr, (PVOID)value, (PVOID)comparand );
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

#include <vector>

class BlockAllocator 
{
   struct block_t
   {
      block_t *next;
   };

   public:
      BlockAllocator( size_t bs )
         : block_size(bs)
         , free_list(nullptr)
      {
         alloc_count = 0;
      }

      ~BlockAllocator()
      {
      }

      void* alloc( size_t const size )
      {
         if (size > block_size) {
            return nullptr; 
         }

         void *ptr = nullptr;

         /** locked version **
         {
            SCOPE_LOCK(lock);
            if (free_list == nullptr) {
               InterlockedIncrement( (LONG*)&alloc_count );
               return ::malloc(block_size);
            } else {
               ptr = free_list;
               free_list = free_list->next;
               return ptr;
            }
         }
         /**/

         /** lockfree free **/
         while (true) {
            block_t *top = free_list;
            if (nullptr == top) {
               InterlockedIncrement( (LONG*)&alloc_count );
               ptr = ::malloc(block_size);
               return ptr;
            }

            ptr = top;
            if (CompareAndSet( &free_list, top, top->next ) == top) {
               return ptr;
            }
         }
         /**/
      }

      void free( void *ptr )
      {
         if (nullptr == ptr) {
            return; 
         }
         
         /* locked *
         {
            SCOPE_LOCK(lock);
            block_t *block = (block_t*) ptr;
            block->next = free_list;
            free_list = block;
         }
         /**/ 

         /* lockless */
         block_t *node = (block_t*)ptr;
         while (true) {
            block_t *top = free_list;
            node->next = top;
            if (CompareAndSet( &free_list, top, node ) == top) {
               break;
            }
         }
         /**/
      }

      template <typename T, typename ...ARGS>
      T* create( ARGS ...args )
      {
         void *buffer = alloc(sizeof(T));
         return new (buffer) T(args...);
      }

      template <typename T>
      void destroy( T *obj ) 
      {
         obj->~T();
         free( obj );
      }

   public:
      size_t block_size;
      block_t *free_list;
      uint alloc_count;

      CriticalSection lock;
};

BlockAllocator gBlocks( ALLOC_SIZE );

//--------------------------------------------------------------------
void* BlockAlloc( size_t size ) 
{
   return gBlocks.alloc( size );
}

//--------------------------------------------------------------------
void BlockFree( void *ptr ) 
{
   gBlocks.free( ptr );
}

class PhysicsObject
{
   public:
      PhysicsObject( float _x, float _y, float _z ) 
         : x(_x), y(_y), z(_z)
         , vx(0), vy(0), vz(0)
         , ax(0), ay(0), az(0)
      {}

      ~PhysicsObject()
      {
         PROFILE_LOG_SCOPE(__FUNCTION__);
      }

      float x, y, z;
      float vx, vy, vz;
      float ax, ay, az;
};

void* operator new( size_t size, BlockAllocator &allocator ) 
{
   return allocator.alloc( size );
}

#define MAX_DEPTH 128
class Callstack
{
   public:
      void *frame_pointers[MAX_DEPTH];
      uint ref_count;

      Callstack()
      {
         ref_count = 1;
      }
};

Callstack* AcquireCallstack( Callstack *cs )
{
   if (nullptr == cs) {
      return nullptr;
   }

   InterlockedIncrement( &cs->ref_count );
   return cs;
}

void ReleaseCallstack( Callstack *cs )
{
   uint ref_count = InterlockedDecrement( &cs->ref_count );
   if (ref_count == 0) {
      delete  cs;
   }
}

// CAS
// InterlockedCompareAndExchange


class SpinLock
{
   public:
      SpinLock()
         : lock_value(0)
      {}

      void lock()
      {
         while (CompareAndSet( &lock_value, 0, 1 ) != 0);
      }

      void unlock()
      {
         lock_value = 0;
      }

   public:
      uint lock_value;
};

class ThreadSafeStack
{
   struct node_t 
   {
      node_t *next;
      uint data;
   };

   public:
      ThreadSafeStack()
         : top(nullptr)
      {}

      void push( uint value )
      {
         node_t *node = new node_t();
         node->data = value;
         node->next = nullptr;

         {
            SCOPE_LOCK(lock);
            node->next = top;
            top = node;
         }
      }

      bool pop( uint *out )
      {
         node_t *old_top = nullptr;

         {
            SCOPE_LOCK(lock);

            if (top == nullptr) {
               return false; 
            }

            old_top = top;
            top = old_top->next;
         }

         *out = old_top->data;
         delete old_top;

         return true;
      }

      inline bool is_empty() const 
      { 
         return top == nullptr; 
      }

   public:
      node_t *top; 
      CriticalSection lock;
};

// 
class LocklessStack 
{
   struct node_t 
   {
      node_t *next;
      uint data;
   };

   public:
      LocklessStack()
         : top(nullptr)
      {}

      void push( uint value )
      {
         node_t *node = new node_t();
         node->data = value;
         node->next = nullptr;

         // Not thread safe version
         while (true) {
            node_t *cur_top = top;
            node->next = cur_top;
            if (CompareAndSet( &top, cur_top, node ) == cur_top) {
               // we will revisit this!
               break;
            }
         }
      }

      bool pop( uint *out )
      {
         // not thread safe version
         while (true) {
            node_t *cur_top = top;
            if (nullptr == cur_top){
               return false;
            }

            if (CompareAndSet( &top, cur_top, cur_top->next ) == cur_top) {
               // old code
               *out = cur_top->data;
               delete cur_top;
               return true;
            }
         }
      }

      inline bool is_empty() const 
      { 
         return top == nullptr; 
      }

   public:
      node_t *top; 
};

//--------------------------------------------------------------------
void StackTest( LocklessStack *stack ) 
{
   uint const TEST_COUNT = 10000;
   for (uint i = 0; i < TEST_COUNT; ++i) {
      stack->push( i );

      uint v;
      if (!stack->pop( &v )) {
         printf( "WTF?\n" );
      }
   }
}

uint AddBytes( void *ptr )
{
   uint v = 0;
   byte_t *c = (byte_t*)ptr;
   for (uint i = 0; i < 512; ++i) {
      v += c[i];
      c[i] = 0;
   }

   return v;
}

//--------------------------------------------------------------------
#pragma optimize( "", off )  
void AllocatorTest( alloc_cb a, free_cb f, int count ) 
{
   int const SIZE = 1024;

   void *blocks[1000];

   uint v = 0;
   for (uint i = 0; i < 1000; ++i) {
      blocks[i] = a(512);
      v += AddBytes(blocks[i]);
   }

   uint total = 0;

   uint iter = count - 1000;
   for (uint i = 0; i < iter; ++i) {
      uint idx = (uint)(rand() % 1000);
      f( blocks[idx] );
      blocks[idx] = a(512);
      v += AddBytes(blocks[idx]);
      
      total += count * i;
   }

   for (uint i = 0; i < 1000; ++i) {
      f( blocks[i] ); 
   }

   printf( "Final value: %u\n", v );
}
#pragma optimize( "", on )  

//--------------------------------------------------------------------
int main( int argc, char const *argv[] ) 
{   
   // run multiple tests - fluctuations in the machine change the result
   // so we want an average one. 
   uint const NUM_TESTS = 4;

   /*
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
   
   // Next, lets see how much adding a "virtual function" adds to this
   // [people say virtual allocs are slow - so.. how slow?]
   for (uint i = 0; i < NUM_TESTS; ++i) {
      PROFILE_LOG_SCOPE("BlockAllocatorTest");
      FunctionPointerTest( BlockAlloc, BlockFree );
   }
   pause();
   */


   // should have resulted in nearly identical results;
   PhysicsObject *po = gBlocks.create<PhysicsObject>( 1.0f, 2.0f, 3.0f );
   gBlocks.destroy( po );
   
   uint const NUM_THREADS = 8;
   thread_handle_t threads[NUM_THREADS];
   int silly_little_value = 0;

   int count = 100000;
   printf( "Enter Count: " );
   scanf( "%i", &count );
   
   LocklessStack stack;
   for (uint i = 0; i < NUM_TESTS; ++i) {
      {
         PROFILE_LOG_SCOPE("MallocTest");
         // AllocatorTest( TestAlloc, TestFree, count );

         /**/
         for (uint i = 0; i < NUM_THREADS; ++i) {
            threads[i] = ThreadCreate( AllocatorTest, TestAlloc, TestFree, count );
         }

         for (uint i = 0; i < NUM_THREADS; ++i) {
            ThreadJoin( threads[i] );
         }
         /**/
      }

      {
         PROFILE_LOG_SCOPE("BlockTest");
         // AllocatorTest( BlockAlloc, BlockFree, count );

         /**/
         for (uint i = 0; i < NUM_THREADS; ++i) {
            threads[i] = ThreadCreate( AllocatorTest, BlockAlloc, BlockFree, count );
         }

         for (uint i = 0; i < NUM_THREADS; ++i) {
            ThreadJoin( threads[i] );
         }
         /**/
      }
   }
   pause();



   return 0;
}


