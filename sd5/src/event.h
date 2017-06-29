#pragma once

// Equivalent of a C# Delegate

/************************************************************************/
/*                                                                      */
/* INCLUDE                                                              */
/*                                                                      */
/************************************************************************/
#include "common.h"
#include "criticalsection.h"

#include <vector>

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

template <typename ...ARGS>
class Event
{
   public:
      // TYPES:
      struct event_sub_t;
      typedef void (*cb_t)( event_sub_t*, ARGS... );
      typedef void (*cb_with_arg_t)( void*, ARGS... );
      // subscription - when subscribing this is the identifying 
      // information (what to call, and what to call with)

      struct event_sub_t 
      {
         cb_t cb;
         void *secondary_cb;
         void *user_arg;
      };

      // STATIC FUNCTIONS AS GO BETWEENS BETWEEN
      // THE EXPECTED CALLBACK AND UNIVERSAL CALLBACK
      static void FunctionWithArgumentCallback( event_sub_t *sub, ARGS ...args );
     
      template <typename T, typename MCB>
      static void MethodCallback( event_sub_t *sub, ARGS ...args );

   public:
      Event()
      {
      }

      ~Event()
      {
         subscriptions.clear();
      }

      // Subscribe a single function (
      void subscribe( void *user_arg, cb_with_arg_t cb ) 
      {
         // Good safeguard in debug to add is to make sure
         // you're not double subscribing to an event
         // with a similar pair. 
         // TODO - ASSERT AGAINST ABOVE

         // Add subscriptoin
         event_sub_t sub;
         sub.cb = FunctionWithArgumentCallback;
         sub.secondary_cb = cb;
         sub.user_arg = user_arg;
         subscriptions.push_back( sub );
      }

      
      // Unsubscribe a function (using user argument as well)
      void unsubscribe( void *user_arg, void* cb ) 
      {
         for (int i = 0; i < subscriptions.size(); ++i) {
            event_sub_t &sub = subscriptions[i];
            if ((sub.secondary_cb == cb) && (sub.user_arg == user_arg)) {
               subscriptions.erase( subscriptions.begin() + i );
               return; // should be unique, so return immeidately
            }
         }
      }

      // remove all subscriptions using this user arg.
      void unsubscribe_by_argument( void *user_arg ) 
      {
         for (int i = 0; i < subscriptions.size(); ++i) {
            event_sub_t &sub = subscriptions[i];
            if (sub.user_arg == obj) {
               // don't return, just remove this object [could do a fast removal if order doesn't matter
               // by just setting last to this and popping back]
               subscriptions.erase( subscriptions.begin() + i );
               --i;
            }
         }
      }

      // Be able to subscribe a method;
      template <typename T>
      void subscribe_method( T *obj, void (T::*mcb)( ARGS... ) )
      {
         event_sub_t sub;
         sub.cb = MethodCallback<T, decltype(mcb)>;
         sub.secondary_cb = *(void**)(&mcb);
         sub.user_arg = obj;
         subscriptions.push_back( sub );
      }

      // unsubscribe - just forwards to normal unsubscribe
      template <typename T>
      void unsubscribe_method( T *obj, void (T::*mcb)( ARGS... ) )
      {
         unsubscribe( obj, *(void**)&mcb );
      }

      // unsubscribe all allocations for this object;
      template <typename T>
      void unsubscribe_object( T *obj ) 
      {
         unsubscribe_by_argument( obj );
      }

      // Triggers the call - calls all registered callbacks;
      void trigger( ARGS ...args )
      {
         for (int i = 0; i < subscriptions.size(); ++i) {
            event_sub_t &sub = subscriptions[i];
            sub.cb( &sub, args... );
         }
      }

   public:
      std::vector<event_sub_t> subscriptions;
};


//--------------------------------------------------------------------
template <typename ...ARGS>
void Event<ARGS...>::FunctionWithArgumentCallback( event_sub_t *sub, ARGS ...args ) 
{
   cb_with_arg_t cb = (cb_with_arg_t)(sub->secondary_cb);
   cb( sub->user_arg, args... );
}

//--------------------------------------------------------------------
template <typename ...ARGS>
template <typename T, typename MCB>
void Event<ARGS...>::MethodCallback( event_sub_t *sub, ARGS ...args )
{
   MCB mcb = *(MCB*)&(sub->secondary_cb);
   T *obj = (T*)(sub->user_arg);
   (obj->*mcb)( args... ); 
}


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
void EventTest();
