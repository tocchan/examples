/************************************************************************/
/*                                                                      */
/* INCLUDE                                                              */
/*                                                                      */
/************************************************************************/
#include "time.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <MMSystem.h>

#pragma comment(lib, "Winmm.lib")

/************************************************************************/
/*                                                                      */
/* DEFINES AND CONSTANTS                                                */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* CLASSES                                                              */
/*                                                                      */
/************************************************************************/
class InternalTimeSystem 
{
   public:
      InternalTimeSystem() 
      {
         ::QueryPerformanceFrequency((LARGE_INTEGER*)&ops_per_second);
         seconds_per_op = 1.0 / (double)ops_per_second;

         ::QueryPerformanceCounter( (LARGE_INTEGER*)&start_ops );
      }

      uint64_t start_ops;
      uint64_t ops_per_second;

      double seconds_per_op;
};


/************************************************************************/
/*                                                                      */
/* LOCAL VARIABLES                                                      */
/*                                                                      */
/************************************************************************/
static InternalTimeSystem gTime;

/************************************************************************/
/*                                                                      */
/* EXTERNAL FUNCTIONS                                                   */
/*                                                                      */
/************************************************************************/

//------------------------------------------------------------------------
//
uint64_t __fastcall TimeGetOpCount()
{
   uint64_t i;
   QueryPerformanceCounter((LARGE_INTEGER*)&i);
   return i;
}

//------------------------------------------------------------------------
uint __fastcall TimeGet_ms()
{
   uint64_t i = TimeGetOpCount() - gTime.start_ops;
   i = (i * 1000U) / gTime.ops_per_second;
   return (uint)i;
}

//------------------------------------------------------------------------
uint __fastcall TimeGet_us()
{
   uint64_t i = TimeGetOpCount() - gTime.start_ops;;
   i = (i * 1000U * 1000U) / gTime.ops_per_second;   // f is operations per second, so I want to return micro seconds, so times the top by 1000000 us / 1 s
   return (uint)i;
}

//------------------------------------------------------------------------
double __fastcall TimeGetSeconds()
{
   uint64_t op = TimeGetOpCount() - gTime.start_ops;
   return (double)op * gTime.seconds_per_op;
}

//------------------------------------------------------------------------
uint64_t TimeOpCountTo_us( uint64_t op_count ) 
{
   op_count *= (1000U * 1000U);
   uint64_t const us = (uint64_t)(op_count * gTime.seconds_per_op);
   return us;
}

//------------------------------------------------------------------------
double TimeOpCountTo_ms( uint64_t op_count ) 
{
   double seconds = op_count * gTime.seconds_per_op;
   return seconds * 1000.0;
}

//------------------------------------------------------------------------
uint64_t TimeOpCountFrom_ms( double ms )
{
   double s = ms / 1000.0;
   uint64_t const ops = (uint64_t)(s * gTime.ops_per_second);
   return ops;
}

//------------------------------------------------------------------------
char const * TimeOpCountToString( uint64_t op_count )
{
   static char buffer[128];
   uint64_t us = TimeOpCountTo_us(op_count);

   if (us < 1500) {
      sprintf_s( buffer, 128, "%llu us", us );
   } else if (us < 1500000) {
      double ms = (double)us / (double)1000.0;
      sprintf_s( buffer, 128, "%.4f ms", ms );
   } else {
      double s = (double)us / (double)(1000000.0);
      sprintf_s( buffer, 128, "%.4f s", s );
   }

   return buffer;
}

