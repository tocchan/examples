/************************************************************************/
/*                                                                      */
/* INCLUDE                                                              */
/*                                                                      */
/************************************************************************/
#include "memory_demo.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <malloc.h>
#include <string>

/************************************************************************/
/*                                                                      */
/* DEFINES AND CONSTANTS                                                */
/*                                                                      */
/************************************************************************/

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

/************************************************************************/
/*                                                                      */
/* LOCAL VARIABLES                                                      */
/*                                                                      */
/************************************************************************/
static byte_t bytes[] = {
   0x89, 0x4c, 0x24, 0x08, 
   0x55, 0x57, 0x48, 0x81, 
   0xec, 0xe8, 0x00, 0x00, 
   0x00, 0x48, 0x8b, 0xec, 
   0x48, 0x8b, 0xfc, 0xb9, 
   0x3a, 0x00, 0x00, 0x00, 
   0xb8, 0xcc, 0xcc, 0xcc, 
   0xcc, 0xf3, 0xab, 0x8b, 
   0x8c, 0x24, 0x08, 0x01,
   0x00, 0x00, 0xc7, 0x45, 
   0x04, 0x07, 0x00, 0x00, // change second one 0x04 -> 0x07 
   0x00, 0x8b, 0x45, 0x04, 
   0x8b, 0x8d, 0x00, 0x01, 
   0x00, 0x00, 0x03, 0xc8, 
   0x8b, 0xc1, 0x89, 0x85, 
   0x00, 0x01, 0x00, 0x00, 
   0x8b, 0x85, 0x00, 0x01,
   0x00, 0x00, 0x48, 0x8d, 
   0xa5, 0xe8, 0x00, 0x00, 
   0x00, 0x5f, 0x5d, 0xc3, 
};

/************************************************************************/
/*                                                                      */
/* GLOBAL VARIABLES                                                     */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* LOCAL FUNCTIONS                                                      */
/*                                                                      */
/************************************************************************/

// Force the compiler not to inline this function
#define NO_INLINE __declspec(noinline)

/************************************************************************/
/*                                                                      */
/* EXTERNAL FUNCTIONS                                                   */
/*                                                                      */
/************************************************************************/

//------------------------------------------------------------------------
NO_INLINE int AddConstant( int a ) 
{
   int value = 4;
   a += value;
   return a;
}

//------------------------------------------------------------------------
NO_INLINE void CastingDemo()
{
   uint byte_count = sizeof(bytes);
   for (uint i = 0; i < byte_count; ++i) {
      printf( "bytes[%i] = %u\n", i, (uint)(bytes[i]) );
   }

   uint *uints = (uint*)bytes;
   uint uint_count = byte_count / sizeof(uint);
   for (uint i = 0; i < uint_count; ++i) {
      printf( "uints[%i] = %u\n", i, uints[i] );
   }
}


class Person
{
   public:
      Person( char const *n, int a ) 
      {
         name = n;
         age = a;
      }

      void speak()
      {
         printf( "%s says: Hello!\n", name.c_str() );
      }

   public:
      std::string name;
      int age;
};

class Dog
{
   public:
      Dog( char const *n ) 
      {
         name = n;
         age = 20;
         bone_buried = 0;
      }

      void bark() 
      {
         printf( "%s barks: Ruff!\n", name.c_str() );
      }

   public:
      std::string name;
      int bone_buried;
      int age;
};


typedef int (*function_cb)(int);

void SomethingAwesome()
{
   function_cb cb = AddConstant;
   cb = (function_cb)(void*)bytes;

   DWORD unused; 
   VirtualProtect( bytes, sizeof(bytes), PAGE_EXECUTE_READWRITE, &unused );

   int a = 20;
   a = AddConstant(a);
   a = cb(a);

   printf( "Value: %u\n", a );
}

int AddConstantHook( int a ) 
{
   // much h4x0r
   a = 0x1337;
   return a;
}

void HookFunction( void *original, void *new_function ) 
{
   int len = 6;

   DWORD old, unused;

   VirtualProtect( original, 6, PAGE_EXECUTE_READWRITE, &old );

   DWORD offset = ((DWORD)new_function - (DWORD)original) - 5;
   byte_t *code = (byte_t*)original;

   code[0] = 0xe9;
   memcpy( code + 1, &offset, sizeof(DWORD) );

   for (uint i = 5; i < len; ++i) {
      *(code + i) = 0x90; // NOP == No OPeration
   }

   VirtualProtect( original, 6, old, &unused );
}

//------------------------------------------------------------------------
void MemoryDemo()
{
   SomethingAwesome();

   int value = 10;
   value = AddConstant( value );

   HookFunction( AddConstant, AddConstantHook );
   value = AddConstant( value );

   printf( "Value: %u\n", value );
}



/************************************************************************/
/*                                                                      */
/* COMMANDS                                                             */
/*                                                                      */
/************************************************************************/

/************************************************************************/
/*                                                                      */
/* UNIT TESTS                                                           */
/*                                                                      */
/************************************************************************/


