#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <conio.h>
#include <stdio.h>

#include "src/common.h"
#include "src/time.h"
#include "src/memory.h"

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

//--------------------------------------------------------------------
int main( int argc, char const *argv[] ) 
{
   int result0;
   int result1;
   uint const iterations = 1000000;

   MemTest();

   // printf( "Enter Iterations: " );
   // scanf( "%u", &iterations );

   {
      PROFILE_LOG_SCOPE("SpeedTest0"); 
      result0 = SpeedTest0( iterations );
   }

   {
      PROFILE_LOG_SCOPE("SpeedTest1"); 
      result1 = SpeedTest1( iterations );
   }

   printf( "Results: %i == %i\n", result0, result1 );

   pause();
   return 0;
}