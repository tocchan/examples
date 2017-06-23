#pragma once

/************************************************************************/
/*                                                                      */
/* INCLUDE                                                              */
/*                                                                      */
/************************************************************************/
#include "common.h"
#include "atomic.h"
#include "util.h"
#include "criticalsection.h"

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

//--------------------------------------------------------------------
//------------------------------------------------------------------------
class IAllocator 
{
   public:
      virtual void* alloc( size_t ) = 0;
      virtual void free( void* ) = 0;

   public:
      // Helpers
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
};

//--------------------------------------------------------------------
//------------------------------------------------------------------------
class BlockAllocator : public IAllocator
{
   struct block_t
   {
      block_t *next;
   };

   public:
      BlockAllocator( size_t bs )
         : free_list(nullptr)
      {
         // must be at least the block pointer size
         block_size = Max( bs, sizeof(block_t) );
      }

      ~BlockAllocator()
      {
      }

      void* alloc( size_t size )
      {
         if (size > block_size) {
            return nullptr; 
         }

         void *ptr = free_list;
         if (nullptr == ptr) {
            ptr = ::malloc(block_size);
            ++alloc_count;
         } else {
            free_list = free_list->next; 
         }

         return ptr;
      }

      void free( void *ptr )
      {
         if (nullptr == ptr) {
            return; 
         }
       
         block_t *block = (block_t*)ptr;
         block->next = free_list;
         free_list = block;
      }


   public:
      size_t block_size;
      block_t *free_list;
      uint alloc_count;
};


//--------------------------------------------------------------------
//--------------------------------------------------------------------
class ThreadSafeBlockAllocator : public IAllocator
{
   struct block_t
   {
      block_t *next;
   };

   public:
      ThreadSafeBlockAllocator( size_t bs )
         : free_list(nullptr)
      {
         block_size = Max( bs, sizeof(block_t) );
      }

      ~ThreadSafeBlockAllocator()
      {
      }

      void* alloc( size_t size )
      {
         if (size > block_size) {
            return nullptr; 
         }

         void *ptr = nullptr;

         {
            SCOPE_LOCK(lock);
            if (free_list == nullptr) {
               ++alloc_count;
               return ::malloc(block_size);
            } else {
               ptr = free_list;
               free_list = free_list->next;
               return ptr;
            }
         }
      }

      void free( void *ptr )
      {
         if (nullptr != ptr) {
            SCOPE_LOCK(lock);
            block_t *block = (block_t*) ptr;
            block->next = free_list;
            free_list = block;
         }
      }


   public:
      size_t block_size;
      block_t *free_list;
      uint alloc_count;

      CriticalSection lock;
};

//--------------------------------------------------------------------
//--------------------------------------------------------------------
class LocklessBlockAllocator : public IAllocator
{
   // on 32-bit, this is a 64-bit structure
   // on 64-bit, this is a 128-bit structure
   // Both these have an CAS instruction associated with them

   struct block_t 
   {
      block_t *next;
   };

   union __declspec(align(16)) node_t
   {
      struct {
         block_t *next;
         uintptr_t aba; // useing a uintptr_t so it is the same size as a pointer
      }; 
      struct {
         // for 32-bit you'd only need one 64-bit value
         uint64_t data[2];
      };
   };


   public:
      LocklessBlockAllocator( size_t bs )
      {
         head.next = nullptr;
         head.aba = 0;
         
         // first difference is that we add
         // our block size to the requested block size, as
         // we are no longer allowed to "reuse" the memory
         block_size = Max( bs, sizeof(block_t) );
         alloc_count = 0;
      }

      ~LocklessBlockAllocator()
      {
      }

      void* alloc( size_t size )
      {
         if (size > block_size) {
            return nullptr; 
         }

         void *ptr = nullptr;

         /** lockfree free **/
         while (true) {
            node_t cur_head = head;
            block_t *top = cur_head.next;

            // list was empty when we checked
            if (nullptr == top) {
               InterlockedIncrement( (LONG*)&alloc_count );
               ptr = ::malloc(block_size);
               return ptr;
            }

            ptr = top;

            node_t new_head;
            new_head.aba = cur_head.aba + 1;
            new_head.next = top->next;

            // ABA problem occurs here.
            // free_list == top, but top->next isn't free_list->next by the time
            // it runs - so may cause corruption
            if (CompareAndSet128( head.data, cur_head.data, new_head.data )) {
               return ptr;
            }
         }
      }

      void free( void *ptr )
      {
         if (nullptr == ptr) {
            return; 
         }

         block_t *node = (block_t*)ptr;
        
         while (true) {
            node_t cur_head = head;

            node->next = cur_head.next;
            node_t new_head; 
            new_head.aba = cur_head.aba + 1;
            new_head.next = node;

            if (CompareAndSet128( head.data, cur_head.data, new_head.data )) {
               break;
            }
         }
      }

   


   public:
      // head MUST be 128 byte aligned;
      node_t head;           // now a full node - we'll be replacing BOTH piece at once
      size_t block_size;      // List of free blocks;

      uint alloc_count;
};

/************************************************************************/
/*                                                                      */
/* GLOBAL VARIABLES                                                     */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* FUNCTION PROTOTYPES                                                  */
/*                                                                      */
/************************************************************************/
