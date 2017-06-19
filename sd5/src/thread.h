#pragma once

/************************************************************************/
/*                                                                      */
/* INCLUDE                                                              */
/*                                                                      */
/************************************************************************/
#include "common.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Used for parameter forwarding
#include <tuple>
#include <utility>

/************************************************************************/
/*                                                                      */
/* DEFINES AND CONSTANTS                                                */
/*                                                                      */
/************************************************************************/
#define INVALID_THREAD_HANDLE 0

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
// empty structs to make C++ tell me if I'm casting wrong.
struct thread_handle_name_t {}; 
struct thread_id_name_t {};

// typedefs, since I'm dealing only with pointers
typedef thread_handle_name_t* thread_handle_t;
typedef thread_id_name_t* thread_id_t;

// Basic callback type.
typedef void (*thread_cb)( void* );

/************************************************************************/
/*                                                                      */
/* STRUCTS                                                              */
/*                                                                      */
/************************************************************************/
//------------------------------------------------------------------------
// Used for parameter forwarding
template <typename CB, typename ...ARGS>
struct pass_data_t
{
   CB cb; 
   std::tuple<ARGS...> args;

   pass_data_t( CB cb, ARGS ...args )
      : cb(cb)
      , args(args...) {}
};

/************************************************************************/
/*                                                                      */
/* CLASSES                                                              */
/*                                                                      */
/************************************************************************/

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
//------------------------------------------------------------------------
// Creates a thread with the entry point of cb, passed data
thread_handle_t ThreadCreate( thread_cb cb, void *data );

void ThreadSleep( uint ms );

// Releases my hold on this thread [one of these MUST be called per create]
void ThreadDetach( thread_handle_t th );
void ThreadJoin( thread_handle_t th );

//------------------------------------------------------------------------
// Get the current thread id
thread_id_t ThreadGetCurrentID();


void ThreadSetNameInVisualStudio( char const *name );

// Demonstration Code
void ThreadDemo();

// Sets teh current threads name - for visual studio

//------------------------------------------------------------------------
// Templated Versions;
//------------------------------------------------------------------------

//------------------------------------------------------------------------
template <typename CB, typename TUPLE, size_t ...INDICES>
void ForwardArgumentsWithIndices( CB cb, TUPLE &args, std::integer_sequence<size_t, INDICES...>& ) 
{
   cb( std::get<INDICES>(args)... );
}

//------------------------------------------------------------------------
template <typename CB, typename ...ARGS>
void ForwardArgumentsThread( void *ptr ) 
{
   pass_data_t<CB,ARGS...> *args = (pass_data_t<CB,ARGS...>*) ptr;
   ForwardArgumentsWithIndices( args->cb, args->args, std::make_index_sequence<sizeof...(ARGS)>() ); 
   delete args;
}

//------------------------------------------------------------------------
template <typename CB, typename ...ARGS>
thread_handle_t ThreadCreate( CB entry_point, ARGS ...args ) 
{
   pass_data_t<CB,ARGS...> *pass = new pass_data_t<CB,ARGS...>( entry_point, args... );
   return ThreadCreate( ForwardArgumentsThread<CB,ARGS...>, (void*)pass );
}


