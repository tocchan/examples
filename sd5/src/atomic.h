#pragma once

/************************************************************************/
/*                                                                      */
/* INCLUDES                                                             */
/*                                                                      */
/************************************************************************/
#include "common.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/************************************************************************/
/*                                                                      */
/* STRUCTS                                                              */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* FUNCTIONS                                                            */
/*                                                                      */
/************************************************************************/

//--------------------------------------------------------------------
// Will return the result of the operation
__forceinline 
uint AtomicAdd( uint volatile *ptr, uint const value ) 
{
   return (uint) ::InterlockedAddNoFence( (LONG volatile*)ptr, (LONG)value );
}

//--------------------------------------------------------------------
__forceinline 
uint AtomicIncrement( uint *ptr ) 
{
   return (uint) ::InterlockedIncrementNoFence( (LONG volatile*)ptr );
}

//--------------------------------------------------------------------
__forceinline 
uint AtomicDecrement( uint *ptr ) 
{
   return (uint) ::InterlockedDecrementNoFence( (LONG volatile*)ptr );
}

//--------------------------------------------------------------------
__forceinline 
uint CompareAndSet( uint volatile *ptr, uint const comparand, uint const value )
{
   /*
   uint const old_value = *ptr;
   if (old_value == comparand) {
      *ptr = value;
   }
   return old_value;
   */

   return ::InterlockedCompareExchange( ptr, value, comparand );
}

//--------------------------------------------------------------------
__forceinline 
bool CompareAndSet128( uint64_t volatile data[2], uint64_t comparand[2], uint64_t value[2] )
{
   return 1 == ::InterlockedCompareExchange128( (long long volatile*)data, value[1], value[0], (long long*)comparand );  
}

//--------------------------------------------------------------------
template <typename T>
__forceinline T* CompareAndSetPointer( T *volatile *ptr, T *comparand, T *value ) 
{
   return (T*)::InterlockedCompareExchangePointerNoFence( (PVOID volatile*)ptr, (PVOID)value, (PVOID)comparand );
}

