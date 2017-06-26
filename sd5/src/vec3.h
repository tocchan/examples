#pragma once

// Standard Library Equivalent:  conditinal variables.

/************************************************************************/
/*                                                                      */
/* INCLUDE                                                              */
/*                                                                      */
/************************************************************************/
#include "common.h"

#include <math.h>

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
struct vec3 
{
   public:
      float x;
      float y;
      float z;

   public:
      vec3() 
         : x(0.0f)
         , y(0.0f)
         , z(0.0f)
      {}

      vec3( float x, float y, float z = 1.0f )
         : x(x)
         , y(y)
         , z(z)
      {}

      explicit vec3( float v )
         : x(v)
         , y(v)
         , z(v)
      {}

      inline float magnitude2() const  { return (x * x) + (y * y) + (z * z); }
      inline float magnitude() const   { return sqrtf( magnitude2() ); }

      void scale( float s ) 
      {
         x *= s;
         y *= s;
         z *= s;
      }

      void scale( vec3 const &s )
      {
         x *= s.x;
         y *= s.y;
         z *= s.z;
      }

      void normalize() 
      {
         float mag = magnitude();
         if (mag > 0.0000001) {
            scale(1.0f / mag);
         } else {
            // default to right vector when normalized - want the post-condition
            // of normalize to always return a unit vector.
            x = 1.0f;
            y = 0.0f;
            z = 0.0f;
         }
      }

      vec3 get_normalized() const 
      {
         vec3 copy = *this;
         copy.normalize();
         return copy;
      }
};

//--------------------------------------------------------------------
// Basic Operators;
inline vec3 operator+( vec3 const &a, vec3 const &b ) 
{
   return vec3( a.x + b.x, a.y + b.y, a.z + b.z );
}

inline vec3 operator-( vec3 const &a, vec3 const &b ) 
{
   return vec3( a.x - b.x, a.y - b.y, a.z - b.z );
}

inline vec3 operator*( vec3 const &v, float const c ) 
{
   vec3 ret = v;
   ret.scale(c);
   return ret;
}

inline vec3 operator*( float const c, vec3 const &v ) 
{
   vec3 ret = v;
   ret.scale(c);
   return ret;
}

inline vec3& operator+=( vec3 &lval, vec3 const &v ) 
{
   lval = lval + v;
   return lval;
}

inline vec3 operator-( vec3 const &v ) 
{
   return vec3( -v.x, -v.y, -v.z );
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

