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
#include "src/random.h"

#include "src/profile.h"
#include "src/job.h"



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
         vec3 disp = pos; // how far are we from the center;

         // do step as normal;
         pos += dt * vel;
         vel += dt * acc;

         // add a gravity well
         float dist2 = disp.magnitude2();
         float grav = 100.0f / dist2; // acceleration inverse of distance
         disp.normalize();
         acc = -disp * grav; // make acc go toward the center.
      }

      particle_t get_updated( float dt ) const
      {
         particle_t ret = *this;
         ret.update(dt);
         return ret;
      }
};

//--------------------------------------------------------------------
static void UpdateParticles( particle_t *particles, uint const count, float const dt ) 
{
   for (uint i = 0; i < count; ++i) {
      particles[i].update(dt);
   }
}

//--------------------------------------------------------------------
// So this one works differently - instead of us giving the thread a 
// chunk of work to do, this will pull chunks of work itself until it detects no 
// work is left.
static void UpdateParticlesInChunks( particle_t *particles, uint *idx_ptr, float dt, uint total ) 
{
   uint const STEP_SIZE = 4096;
   
   uint idx = AtomicAdd( idx_ptr, STEP_SIZE ) - STEP_SIZE; 
   while (idx < total) {
      uint stop = min( total, idx + STEP_SIZE );
      for (uint i = idx; i < stop; ++i) {
         particles[idx].update(dt);
      }

      idx = AtomicAdd( idx_ptr, STEP_SIZE ) - STEP_SIZE; 
   }
}

//--------------------------------------------------------------------
void EmptyThread( uint *ptr )
{
   // I do basically nothing!
   AtomicIncrement( ptr );
}

//--------------------------------------------------------------------
void CalculatePrimes( uint count, uint max_number ) 
{
   PROFILE_LOG_SCOPE("CalculatePrimes"); 

   // one is always our first prime.
   uint *primes = new uint[count];

   primes[0] = 1;

   uint c = 0;
   for (uint n = 2; n <= max_number; ++n) {
      // skip first prime
      bool is_prime = true;
      for (uint i = 1; i < c; ++i) {
         if ((n % primes[i]) == 0) {
            // not a prime - can be divided by a previous prime
            is_prime = false;
            break;
         }
      }

      if (is_prime) {
         primes[c] = n;
         ++c;
         if (c == count) {
            break;
         }
      }
   }

   delete[] primes;
}

#include "src/event.h"

//--------------------------------------------------------------------
int main( int argc, char const *argv[] ) 
{   
   EventTest();
   pause();

   JobSystemTest();







   uint const NUM_TESTS = 10;
   uint const NUM_THREADS = 8;

   uint const CONTENDING_THREADS = 0;
   uint const MAX_PRIMES = 100000; // in release, calculating 100000 primes took 18 seconds

   // Doing 10 million particles in release, 1 million in debug.
   #if defined(_DEBUG)
      uint const NUM_PARTICLES = 1000000;
   #else 
      uint const NUM_PARTICLES = 10000000;
   #endif

   
   for (uint i = 0; i < CONTENDING_THREADS; ++i) {
      thread_handle_t th = ThreadCreate( CalculatePrimes, MAX_PRIMES, (uint) 0x7fffffff );
      ThreadDetach(th);
   }

   particle_t *particles = new particle_t[NUM_PARTICLES];

   {
      // Takes about a second on my home machine.
      PROFILE_LOG_SCOPE("Particles Init");
      // just initializing some noisy particles in a cube 10 long;
      srand( (uint)TimeGetOpCount() );
      for (uint i = 0; i < NUM_PARTICLES; ++i) {
         particles[i].pos = 5.0f * RandomInUnitCube();
         particles[i].vel = 4.0f * RandomInUnitCube(); 
      }
   }


   // First, let's time how long it takes to create threads;
   uint const TOTAL_THREADS = 1000;
   std::vector<thread_handle_t> thread_test;
   thread_test.reserve(TOTAL_THREADS);

   {
      PROFILE_LOG_SCOPE( "1000 Atomics" );
      uint count = 0;
      for (uint i = 0; i < 1000; ++i) {
         AtomicIncrement( &count ); 
      }
   }

   {
      PROFILE_LOG_SCOPE( "1000 Threads" );
      uint count = 0;
      for (uint i = 0; i < TOTAL_THREADS; ++i) {
         thread_test.push_back( ThreadCreate( EmptyThread, &count ) );
      }

      while (count < TOTAL_THREADS) {
         ThreadYield(); 
      }
   }
   ThreadJoin( &thread_test[0], (uint) thread_test.size() );

   pause();
   printf( "\n" );


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
      {
         PROFILE_LOG_SCOPE("Particle Update :: MultiThread");
         thread_handle_t threads[NUM_THREADS];
         uint thread_part_count = NUM_PARTICLES / (NUM_THREADS + 1); // +1 because the main thread is going to do some work too.
         uint start_idx = 0;

         for (uint i = 0; i < NUM_THREADS; ++i) {
            threads[i] = ThreadCreate( UpdateParticles, particles + start_idx, thread_part_count, dt );
            start_idx += thread_part_count;
         }

         // while they're doing work - we should do work too! So do the remaining.
         UpdateParticles( particles + start_idx, NUM_PARTICLES - start_idx, dt );

         // wait for everyone to finish
         ThreadJoin( threads, NUM_THREADS );
      }

      // sample 3 : multi thread test - benchmark - how much does just spinning up a thread add?
      // Okay, this is worse - but as our "update" function gets more complicated, this starts getting better
      // meaning memory contention is likely our issue;
      {
         PROFILE_LOG_SCOPE("Particle Update :: Chunked");

         // Okay, let's try something else;
         thread_handle_t threads[NUM_THREADS];
         uint idx = 0;

         for (uint i = 0; i < NUM_THREADS; ++i) {
            threads[i] = ThreadCreate( UpdateParticlesInChunks, particles, &idx, dt, NUM_PARTICLES );
         }

         // while they're doing work - we should do work too! So do the remaining.
         UpdateParticlesInChunks( particles, &idx, dt, NUM_PARTICLES );

         // wait for everyone to finish - just because I ran out of work doesn't mean they're down with their acquired chunks
         ThreadJoin( threads, NUM_THREADS );
      }

      // new line - space out each test.
      printf("\n");
   }


   // a single update is about 4ms on my machine;
   pause();

   delete[] particles;
   return 0;
}


