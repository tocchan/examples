#pragma once

/************************************************************************/
/*                                                                      */
/* INCLUDE                                                              */
/*                                                                      */
/************************************************************************/
#include "common.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/************************************************************************/
/*                                                                      */
/* DEFINES AND CONSTANTS                                                */
/*                                                                      */
/************************************************************************/
// Infoknowledge Management System

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
typedef void* thread_handle_t;
typedef void (*thread_cb)( void* );

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

// Mutual Exclusive [mutex]: 
// Fair:  Serviced in order of request.
// Recursive: Someone is allowed to lock multiple times.
class CriticalSection 
{
   public:
      CriticalSection()
      {
         InitializeCriticalSection( &cs );
      }

      ~CriticalSection()
      {
         DeleteCriticalSection( &cs );
      }

      void lock()
      {
         EnterCriticalSection( &cs );
      }

      void unlock()
      {
         LeaveCriticalSection( &cs );
      }

   public: 
      CRITICAL_SECTION cs;
};

class ScopeCriticalSection
{
   public:
      ScopeCriticalSection( CriticalSection *ptr ) 
      {
         cs_ptr = ptr;
         cs_ptr->lock();
      }

      ~ScopeCriticalSection()
      {
         cs_ptr->unlock();
      }

   public:
      CriticalSection *cs_ptr;
};

#define COMBINE1(X,Y) X##Y
#define COMBINE(X,Y) COMBINE1(X,Y)
#define SCOPE_LOCK( csp ) ScopeCriticalSection COMBINE(__scs_,__LINE__)(csp)

#include <queue>

template <typename T>
class ThreadSafeQueue
{
   public:
      void push( T const &v ) 
      {
         SCOPE_LOCK(&m_lock);
         m_queue.push( v );
      }

      bool empty() 
      {
         SCOPE_LOCK(&m_lock);
         bool result = m_queue.empty();

         return result;
      }

      bool pop( T *out )
      {
         SCOPE_LOCK(&m_lock);

         if (m_queue.empty()) {
            return false; 
         } else {
            *out = m_queue.front();
            m_queue.pop();
            return true;
         }
      }

      T front() 
      {
         SCOPE_LOCK(&m_lock);
         T result = m_queue.front(); 

         return result;
      }

   public:
      std::queue<T> m_queue;
      CriticalSection m_lock;
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

// Creates a thread with the entry point of cb, passed data
thread_handle_t ThreadCreate( thread_cb cb, void *data );

void ThreadSleep( uint ms );

// Releases my hold on this thread [one of these MUST be called per create]
void ThreadDetach( thread_handle_t th );
void ThreadJoin( thread_handle_t th );

void ThreadDemo();

