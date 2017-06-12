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

//--------------------------------------------------------------------
// EVENT VERSION 0
// Basic Idea in place.  
// - Not Thread Safe
// - Only works with void* data
// - Only works with C style functions
//--------------------------------------------------------------------

// An Event/Delegate at its core is just a list of function callbacks
//
// Events can be registered with some identifying information [user_arg].
// And calling the event can take some event specific data [event_arg]
typedef void (*basic_event_cb)( void *user_arg, void *event_arg );

class EventV0
{
   // subscription - when subscribing this is the identifying 
   // information (what to call, and what to call with)
   struct event_sub_t 
   {
      basic_event_cb cb;
      void *user_arg;
   };

   public:
      EventV0()
      {
      }

      ~EventV0()
      {
         subscriptions.clear();
      }

      void subscribe( void *user_arg, basic_event_cb cb ) 
      {
         // Good safeguard in debug to add is to make sure
         // you're not double subscribing to an event
         // with a similar pair. 
         // TODO - ASSERT AGAINST ABOVE

         // Add subscriptoin
         event_sub_t sub;
         sub.cb = cb;
         sub.user_arg = user_arg;
         subscriptions.push_back( sub );
      }

      void unsubscribe( void *user_arg, basic_event_cb cb ) 
      {
         for (int i = 0; i < subscriptions.size(); ++i) {
            event_sub_t &sub = subscriptions[i];
            if ((sub.cb == cb) && (sub.user_arg == user_arg)) {
               subscriptions.erase( subscriptions.begin() + i );
               return; // should be unique, so return immeidately
            }
         }
      }

      void trigger( void *event_arg ) 
      {
         for (int i = 0; i < subscriptions.size(); ++i) {
            event_sub_t &sub = subscriptions[i];
            sub.cb( sub.user_arg, event_arg );
         }
      }

   public:
      std::vector<event_sub_t> subscriptions;
};

//--------------------------------------------------------------------
// EVENT VERSION 1
// - Thread Safety around 
// - Add support for callbacks without user data
// - Adds supports for methods
//--------------------------------------------------------------------
// TODO

//--------------------------------------------------------------------
// EVENT VERSION 2
// - Templated argument types
//--------------------------------------------------------------------
// TODO

//--------------------------------------------------------------------
// EVENT VERSION 3
// - Thread Safety [ReaderWriter Locks]
// - 
//--------------------------------------------------------------------
// Thread Safe

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


