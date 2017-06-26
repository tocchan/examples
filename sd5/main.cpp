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
#include "src/blockallocator.h"
#include "src/ts_queue.h"
#include "src/vec3.h"

#include "src/profile.h"



//--------------------------------------------------------------------
// JOB SYSTEM STUFF
//--------------------------------------------------------------------



//--------------------------------------------------------------------
struct particle_t
{
   public:
      vec3 pos;
      vec3 vel;
      vec3 acc;

   public:
      particle_t()
      {
         acc = vec3( 0.0f, 0.0f, -9.8f );
      }

      void update( float dt ) 
      {
         // add a gravity well
         vec3 disp = pos; // how far are we from the center;
         float dist2 = disp.magnitude2();
         float grav = 100.0f / dist2; // acceleration inverse of distance
         disp.normalize();
         acc = -disp * grav; // make acc go toward the center.

         // do step as normal;
         vel += dt * acc;
         pos += dt * vel;
      }
};

//--------------------------------------------------------------------
// return a random float between 0.0f and 1.0f;
float RandomFl() 
{
   return (float)rand() / (float)RAND_MAX;
}

float RandomFl( float min, float max ) 
{
   float v = RandomFl();
   return min + (max - min) * v;
}

vec3 RandomInUnitCube()
{
   return vec3( RandomFl( -1.0f, 1.0f ), 
      RandomFl( -1.0f, 1.0f ), 
      RandomFl( -1.0f, 1.0f ) );
}

//--------------------------------------------------------------------
static void UpdateParticles( particle_t *particles, uint const count, float const dt ) 
{
   char buffer[128];
   sprintf_s( buffer, 128, "Particle Update :: %u", ThreadGetCurrentID() );
   PROFILE_LOG_SCOPE(buffer);

   for (uint i = 0; i < count; ++i) {
      particles[i].update(dt);
   }
}

//--------------------------------------------------------------------
int main( int argc, char const *argv[] ) 
{   
   srand( (uint)TimeGetOpCount() );

   uint const NUM_TESTS = 10;

   // Doing 10 million particles in release, 1 million in debug.
   #if defined(_DEBUG)
      uint const NUM_PARTICLES = 1000000;
   #else 
      uint const NUM_PARTICLES = 10000000;
   #endif

   particle_t *particles = new particle_t[NUM_PARTICLES];

   {
      // Takes about a second on my home machine.
      PROFILE_LOG_SCOPE("Particles Init");
      // just initializing some noisy particles in a cube 10 long;
      for (uint i = 0; i < NUM_PARTICLES; ++i) {
         particles[i].pos = 5.0f * RandomInUnitCube();
         particles[i].vel = 4.0f * RandomInUnitCube(); 
      }
   }
   pause();

   float dt = 0.0f;

   for (uint testi = 0; testi < NUM_TESTS; ++testi) {
      // Okay, so going for 60 frames per second.
      // add some noise to it to prevent compiler from optimizing for a constnat;
      dt = (1.0f / 60.0f) + RandomFl( -0.001f, 0.001f );

      // sample 0 - main thread test
      {
         PROFILE_LOG_SCOPE("Particles Update :: Main Thread");
         UpdateParticles( particles, NUM_PARTICLES, dt );
      }

      // sample 1 : single thread test - benchmark - how much does just spinning up a thread add?
      // and splitting the load.
      {
         PROFILE_LOG_SCOPE("Particle Update :: One Thread");
         // Okay, let's try something else;
         // sample 1 - single thread
         uint thread_part_count = NUM_PARTICLES / 2;
         thread_handle_t th = ThreadCreate( UpdateParticles, particles, thread_part_count, dt );

         // while they're doing work - we should do work too! 
         UpdateParticles( particles + thread_part_count, NUM_PARTICLES - thread_part_count, dt );

         ThreadJoin(th);
      }

      // sample 2 : multi thread test - benchmark - how much does just spinning up a thread add?
      // Okay, this is worse - but as our "update" function gets more complicated, this starts getting better
      // meaning memory contention is likely our issue;
      uint const NUM_THREADS = 8;
      {
         PROFILE_LOG_SCOPE("Particle Update :: MultiThread");
         // Okay, let's try something else;
         // sample 1 - single thread
         thread_handle_t threads[NUM_THREADS];
         uint thread_part_count = NUM_PARTICLES / (NUM_THREADS); // +1 because the main thread is going to do some work too.
         uint start_idx = 0;

         for (uint i = 0; i < NUM_THREADS; ++i) {
            threads[i] = ThreadCreate( UpdateParticles, particles + start_idx, thread_part_count, dt );
            start_idx += thread_part_count;
         }

         // while they're doing work - we should do work too! So do the remaining.
         // UpdateParticles( particles + start_idx, NUM_PARTICLES - start_idx, dt );

         // wait for everyone to finish
         ThreadJoin( threads, NUM_THREADS );
      }


      // add a space
      printf( "\n" );
   }

   // a single update is about 4ms on my machine;
   pause();

   delete[] particles;
   return 0;
}


