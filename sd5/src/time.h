#pragma once
#if !defined( __ITW_TIME_TIME__ )
#define __ITW_TIME_TIME__ 

/************************************************************************/
/*                                                                      */
/* INCLUDES                                                             */
/*                                                                      */
/************************************************************************/
#include "common.h"

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
// High performance timer
uint64_t __fastcall TimeGetOpCount();
uint __fastcall TimeGet_ms();
uint __fastcall TimeGet_us();
double __fastcall TimeGetSeconds();

uint64_t TimeOpCountTo_us( uint64_t op_count );
double TimeOpCountTo_ms( uint64_t op_count );
uint64_t TimeOpCountFrom_ms( double ms );

// NOT THREAD SAFE
char const* TimeOpCountToString( uint64_t op_count );


#endif