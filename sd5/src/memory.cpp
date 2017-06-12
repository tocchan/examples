/************************************************************************/
/*                                                                      */
/* INCLUDE                                                              */
/*                                                                      */
/************************************************************************/
#include "memory.h"

#include <malloc.h>

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
static uint gAllocCount = 0;
static uint gFrameAllocs = 0;
static uint gFrameFrees = 0;
static size_t gAllocatedByteCount = 0;

struct allocation_t 
{
   size_t byte_size;
};

/*
void* operator new( size_t const size ) 
{
   ++gAllocCount;
   ++gFrameAllocs;
   gAllocatedByteCount += size;

   size_t alloc_size = size + sizeof(allocation_t);
   allocation_t *ptr = (allocation_t*) malloc(alloc_size);
   ptr->byte_size = size;
   return ptr + 1;
}

void operator delete( void *ptr ) 
{
   --gAllocCount;
   ++gFrameFrees;

   allocation_t *size_ptr = (allocation_t*) ptr;
   size_ptr--;

   gAllocatedByteCount -= size_ptr->byte_size;
   free( size_ptr );
}
*/

class Foo
{
   public:
      Foo() 
      {
         printf( "Foo::ctor\n" );
      }

      ~Foo()
      {
         printf( "Foo::dtor\n" );
      }
};

struct uint3 
{
   uint x;
   uint y;
   uint z;
};

void ProfileMemoryFrameTick()
{
   gFrameAllocs = 0;
   gFrameFrees = 0;
}

#include <map>
#include <string>
#include <random>

void MemTest()
{
   int *array = new int[1000];
   uint3 *f3 = new uint3();
   f3->x = 1;
   f3->y = 2;
   f3->z = 3;

   byte_t buffer[128];
   Foo *test = new (buffer) Foo();
   test->~Foo();

   delete [] array;
   delete f3;
}
uint GetAllocCount()
{
   return gAllocCount;
}

/*
void PrintLiveAllocations()
{
   for each (allocation) {
      printf( "Allocation at 0x%08x took %u B\n", 
         allocation->ptr, 
         allocation->size ); 
   }
}
*/

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


