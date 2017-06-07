#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <conio.h>
#include <stdio.h>

#include "src/common.h"
#include "src/time.h"
#include "src/memory.h"
#include "src/memory_demo.h"
#include "src/thread.h"

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
   MemoryDemo();
   ThreadDemo();
   
   pause();
   return 0;
}