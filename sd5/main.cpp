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



//--------------------------------------------------------------------
int SpeedTest0( uint const iterations )
{
   gA = 0;
   for (uint i = 0; i < iterations; ++i) {
      gA += 3;
      gA += i;
      gA -= 2;
   }

   return gA;
}


//--------------------------------------------------------------------
int SpeedTest1( uint const iterations ) 
{
   gB = 0;
   for (uint i = 0; i < iterations; ++i) {
      gB += 3;
      gB += i;
      gB -= 2;
   }

   return gB;
}

void SomeThreadFunction( int a, float b, char const *s ) 
{
}

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

void PrintNumber( int c ) 
{
   printf( "%i\n", c );
}

#include <tuple>
#include <utility>

template <typename CB, typename ...ARGS>
struct pass_data_t
{
   CB cb; 
   std::tuple<ARGS...> args;

   pass_data_t( CB cb, ARGS ...args )
      : cb(cb)
      , args(args...) {}
};

template <typename CB, typename TUPLE, size_t ...INDICES>
void ForwardArgumentsWithIndices( CB cb, TUPLE &args, std::integer_sequence<size_t, INDICES...>& ) 
{
   cb( std::get<INDICES>(args)... );
}

template <typename CB, typename ...ARGS>
void ForwardArgumentsThread( void *ptr ) 
{
   pass_data_t<CB,ARGS...> *args = (pass_data_t<CB,ARGS...>*) ptr;
   ForwardArgumentsWithIndices( args->cb, args->args, std::make_index_sequence<sizeof...(ARGS)>() ); 
   delete args;
}

template <typename CB, typename ...ARGS>
thread_handle_t ThreadCreate( CB entry_point, ARGS ...args ) 
{
   pass_data_t<CB,ARGS...> *pass = new pass_data_t<CB,ARGS...>( entry_point, args... );
   return ThreadCreate( ForwardArgumentsThread<CB,ARGS...>, (void*)pass );
}

void ThreadDoWork( int a, float b, char const *c ) 
{
   printf( "%i, %.4f, %s", a, b, c );
}

void TemplateWorkshop()
{
   int a = 1;
   float b = 2.0f;
   char const *c = "Chris the Hedgehog";

   // Foo( 1 );
   // int c = Max( 4,  10 );

   int max = Max( 4, 2, 3, 7, 3, 5 );
   ThreadCreate( ThreadDoWork, 1, 2.0f, "Hello" );

   //
   // ThreadCreate( SomeThreadFunction, 1, 3.0f, "hello" );
}





#include "src/event.h"

void SomeEventCB( void*, int a, int b ) 
{
   printf( "%i\n", Max(a, b) );
}

//--------------------------------------------------------------------
int main( int argc, char const *argv[] ) 
{
   // MemoryDemo();
   // pause();
   
   TemplateWorkshop();
   EventV1<int,int> some_event;
   some_event.subscribe( nullptr, SomeEventCB );
   some_event.trigger( 3, 5 );
   
   SignalTest();
   pause();

   ThreadDemo();
   pause();
   return 0;
}