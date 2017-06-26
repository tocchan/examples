/************************************************************************/
/*                                                                      */
/* INCLUDE                                                              */
/*                                                                      */
/************************************************************************/
#include "blockallocator.h"

#include "profile.h"
#include "thread.h"
#include "ts_queue.h"

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
class SystemAllocator : public IAllocator 
{
   public:
      void* alloc( size_t size ) { return ::malloc(size); }
      void free( void *ptr ) { return ::free(ptr); }
};

/************************************************************************/
/*                                                                      */
/* LOCAL VARIABLES                                                      */
/*                                                                      */
/************************************************************************/
static int const BLOCK_SIZE = 1024;
static uint const NUM_QUEUES = 8;

static SystemAllocator gSystemAllocator;
static BlockAllocator gBlockAllocator(BLOCK_SIZE);
static ThreadSafeBlockAllocator gTSBlockAllocator(BLOCK_SIZE);
static LocklessBlockAllocator gLFBlockAllocator(BLOCK_SIZE);

static ThreadSafeQueue<void*> gQueues[NUM_QUEUES];

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
//--------------------------------------------------------------------
static uint AddBytes( void *ptr )
{
   uint v = 0;
   byte_t *c = (byte_t*)ptr;
   for (uint i = 0; i < 512; ++i) {
      v += c[i];
      c[i] = (byte_t)(2 * c[i] + 3);
   }

   return v;
}


//--------------------------------------------------------------------
#pragma optimize( "", off )  
static void AllocatorTest( IAllocator *allocator, uint count ) 
{

   uint value = 0;
   void *ptr = nullptr;

   // Run through this count times, randomly selecting a queue
   // reading from it, and free if we succeed
   // then make a new allocation and push it;
   // This is to prvent compiler from realizing all allocations are created within the same scope
   for (uint i = 0; i < count; ++i) {

      int queue_idx = rand() % NUM_QUEUES;
      ThreadSafeQueue<void*> *q = gQueues + queue_idx;

      if (q->dequeue(&ptr)) {
         value += AddBytes(ptr);
         allocator->free(ptr);
      }

      ptr = allocator->alloc(BLOCK_SIZE);
      q->enqueue(ptr);
   }

   // once we're done, make sure all the queues are empty
   for (uint i = 0; i < NUM_QUEUES; ++i) {
      ThreadSafeQueue<void*> *q = gQueues + i;
      while (q->dequeue(&ptr)) {
         value += AddBytes(ptr);
         allocator->free(ptr); 
      }
   }

   // printf( "Final Value: %i\n", value );
}
#pragma optimize( "", on )  

/************************************************************************/
/*                                                                      */
/* EXTERNAL FUNCTIONS                                                   */
/*                                                                      */
/************************************************************************/



//--------------------------------------------------------------------
void RunAllocatorSpeedTest()
{
     // run multiple tests - fluctuations in the machine change the result
   // so we want an average one. 
   uint const NUM_TESTS = 4;

   uint const NUM_THREADS = 8;
   thread_handle_t threads[NUM_THREADS];

   int count = 100000;
   printf( "Enter Count: " );
   scanf_s( "%i", &count );

   // Single threaded test.
   printf( "Single Threaded Test...\n" );
   for (uint i = 0; i < NUM_TESTS; ++i) {
      {
         PROFILE_LOG_SCOPE("Malloc - Single");
         AllocatorTest( &gSystemAllocator, count );
      }

      {
         PROFILE_LOG_SCOPE("Block - Single");
         AllocatorTest( &gBlockAllocator, count );
      }
   }
   pause();

   // Multi threaded test...
   printf( "Multi-threaded Test...\n" );
   for (uint i = 0; i < NUM_TESTS; ++i) {
      {
         PROFILE_LOG_SCOPE("MallocTest");
         for (uint i = 0; i < NUM_THREADS; ++i) {
            threads[i] = ThreadCreate( AllocatorTest, &gSystemAllocator, count );
         }

         for (uint i = 0; i < NUM_THREADS; ++i) {
            ThreadJoin( threads[i] );
         }
      }

      {
         PROFILE_LOG_SCOPE("LockedAllocator");
         for (uint i = 0; i < NUM_THREADS; ++i) {
            threads[i] = ThreadCreate( AllocatorTest, &gTSBlockAllocator, count );
         }

         for (uint i = 0; i < NUM_THREADS; ++i) {
            ThreadJoin( threads[i] );
         }
      }

      {
         PROFILE_LOG_SCOPE("LockFreeAllocator");
         for (uint i = 0; i < NUM_THREADS; ++i) {
            threads[i] = ThreadCreate( AllocatorTest, &gLFBlockAllocator, count );
         }

         for (uint i = 0; i < NUM_THREADS; ++i) {
            ThreadJoin( threads[i] );
         }
      }
   }
   printf( "Max Allocations: %u\n", gTSBlockAllocator.alloc_count );
   pause();
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
