/************************************************************************/
/*                                                                      */
/* INCLUDE                                                              */
/*                                                                      */
/************************************************************************/
#include "job.h"

#include "ts_queue.h"
#include "signal.h"
#include "atomic.h"
#include "thread.h"
#include "profile.h"

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
class JobSystem
{
   public:
      JobQueue *queues;
      Signal **signals;
      uint queue_count;

      bool is_running;
};

/************************************************************************/
/*                                                                      */
/* LOCAL VARIABLES                                                      */
/*                                                                      */
/************************************************************************/
static JobSystem *gJobSystem = nullptr;

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

//------------------------------------------------------------------------
static void GenericJobThread( Signal *signal ) 
{
   JobConsumer consumer;
   consumer.add_category( JOB_GENERIC );

   while (gJobSystem->is_running) {
      signal->wait();
      consumer.consume_all_jobs();
   }

   consumer.consume_all_jobs();
}

/************************************************************************/
/*                                                                      */
/* GLOBAL FUNCTIONS                                                     */
/*                                                                      */
/************************************************************************/

//------------------------------------------------------------------------
void Job::on_finish()
{
   // we're done - do this first - as anyone waiting 
   // doesn't care about my dependendants, so we'll let them know as soon as possible.
   set_state( JOB_STATE_FINISHED );

   // inform our dependants that we are done.
   for (uint i = 0; i < dependents.size(); ++i) {
      dependents[i]->on_dependancy_finished();
   }
   
   // in the case I recycle these - want to make sure this starts cleared.
   dependents.clear();
}

//------------------------------------------------------------------------
void Job::on_dependancy_finished() 
{
   // we both dispatch and release [the dependancy's hold on me]
   // You'll notice ref_count should almost be >= than dependancy_count due to this;
   JobDispatchAndRelease( this );
}

//------------------------------------------------------------------------
void Job::dependent_on( Job *parent ) 
{
   // TODO:  Only allow this if the parent has not yet been dispatched once
   // [and warning/assert if not].  May require an additional state.

   // I have a no dependancy, increment the count
   AtomicIncrement( &dependancy_count );

   // Increment my reference count, as this parent is now holding on to me as well.
   JobAcquire( this );

   // Push it back
   parent->dependents.push_back( this );
}

//------------------------------------------------------------------------
void Job::set_state( eJobState new_state ) 
{
   state = new_state;
}

//------------------------------------------------------------------------
// JobConsumer
//------------------------------------------------------------------------

//------------------------------------------------------------------------
void JobConsumer::add_category( uint category )
{
   JobQueue *queue = JobSystemGetQueue( category );
   if (nullptr == queue) {
      // error/warning -> category didn't exist
      return; 
   }

   // Add uniquely!  Takes no time to search it, and consumers
   // should be setup at init time, so the cost doesn't matter.
   if (std::find( queues.begin(), queues.end(), queue ) == queues.end()) {
      queues.push_back( queue );
   }
}

//------------------------------------------------------------------------
bool JobConsumer::consume_job()
{
   Job *job = nullptr;
   for (uint i = 0; i < queues.size(); ++i) {
      JobQueue *queue = queues[i];
      if (queue->dequeue( &job )) {

         // queue no longer holds it, and now the consumer does
         // so instead of acquiring and releasing, I just do nothing
         // as the reference silently passes between the two states.
         job->set_state( JOB_STATE_RUNNING );

         // Do the work
         job->work_cb( job->user_data );

         // And we're done.
         job->on_finish();
         job->dependents.clear();
         
         // release my hold on this job.
         JobRelease(job);
         return true;
      }
   }

   // no queues had work for me - then this failed
   return false;
}

//------------------------------------------------------------------------
uint JobConsumer::consume_all_jobs()
{
   // all work is in consume job - so.. just do that while it succeeds.
   uint count = 0;
   while (consume_job()) {
      ++count;
   }

   return count;
}

//------------------------------------------------------------------------
uint JobConsumer::consume_for_ms( uint ms )
{
   uint count = 0;
   uint start = TimeGet_ms();
   do {
      // consume a job - if we fail - just return, no reason to busy loop.
      if (!consume_job()) {
         return count; 
      }
      // otherwise (we consumed a job), increment the count and see if we've elapsed time.
      ++count;
   } while ((TimeGet_ms() - start) < ms);

   return count;
}

//------------------------------------------------------------------------
void JobSystemStartup( uint job_category_count, int generic_thread_count /*= -1*/ )
{
   int core_count = (int)std::thread::hardware_concurrency();
   if (generic_thread_count <= 0) {
      core_count += generic_thread_count; 
   }
   core_count--; // one is always being created - so subtract from total wanted;

   // We need queues! 
   gJobSystem = new JobSystem();
   gJobSystem->queues = new ThreadSafeQueue<Job*>[job_category_count];
   gJobSystem->signals = new Signal*[job_category_count];
   gJobSystem->queue_count = job_category_count;
   gJobSystem->is_running = true;

   for (uint i = 0; i < job_category_count; ++i) {
      gJobSystem->signals[i] = nullptr;
   }

   // create the signal
   gJobSystem->signals[JOB_GENERIC] = new Signal();

   ThreadCreate( GenericJobThread, gJobSystem->signals[JOB_GENERIC] );
   for (int i = 0; i < core_count; ++i) {
      ThreadCreate( GenericJobThread, gJobSystem->signals[JOB_GENERIC] );
   }
}

//------------------------------------------------------------------------
void JobSystemShutdown()
{
   // dont' forget to clean up.
   // TODO!

   // You should stop the system
   // and ensure all enqueued jobs have finished.
}

//------------------------------------------------------------------------
void JobSystemSetSignal( uint category, Signal *signal )
{
   // not a proper category?  Return
   if (category >= gJobSystem->queue_count) {
      return; 
   }

   gJobSystem->signals[category] = signal;
}

//------------------------------------------------------------------------
JobQueue* JobSystemGetQueue(uint category)
{
   // not a proper category?  Return
   if (category >= gJobSystem->queue_count) {
      return nullptr;
   }

   // notice I return a POINTER to the queue
   // there is only ever one unique queue, and the consumers just reference it.
   return &(gJobSystem->queues[category]);
}


//------------------------------------------------------------------------
Job* JobCreate( eJobType type, job_work_cb work_cb, void *user_data )
{
   Job *job = new Job();
   job->type = type;
   job->state = JOB_STATE_WAITING;
   job->work_cb = work_cb;
   job->user_data = user_data;
   job->dependancy_count = 1;
   job->ref_count = 1;

   return job;
}

//------------------------------------------------------------------------
void JobAcquire( Job *job )
{
   AtomicIncrement( &job->ref_count );
}

//------------------------------------------------------------------------
void JobRelease( Job *job )
{
   // remove a reference - if we're the last one, delete me!
   uint ref_count = AtomicDecrement( &job->ref_count );
   if (0 == ref_count) {
      delete job;
   }
}

//------------------------------------------------------------------------
void JobDispatch( Job *job )
{
   // if I'm not ready to run, don't. 
   uint dcount = AtomicDecrement( &job->dependancy_count );
   if (dcount != 0) {
      return; 
   }

   // update my state
   job->state = JOB_STATE_ENQUEUED; 

   // I'm being qneueued - so the queue now holds a reference to me
   // do this BEFORE qneueing to prevent the job system
   // from potentially releasing before I have a chance to acquire.
   JobAcquire( job );
   gJobSystem->queues[job->type].enqueue( job );

   // IF a signal is associated with this job type, signal it.
   Signal *signal = gJobSystem->signals[job->type];
   if (nullptr != signal) {
      signal->signal_all();
   }
}

//------------------------------------------------------------------------
void JobDispatchAndRelease( Job *job )
{
   // exactly as the name says.
   JobDispatch( job );
   JobRelease( job );
}


//------------------------------------------------------------------------
void JobWait( Job *job, JobConsumer *consumer )
{
   while (!job->is_finished()) {
      if (consumer != nullptr) {
         consumer->consume_job(); 
      } 
   }
}

//------------------------------------------------------------------------
void JobWaitAndRelease( Job *job, JobConsumer *consumer )
{
   JobWait( job, consumer );
   JobRelease( job );
}


//--------------------------------------------------------------------
// Some test code - make sure I did the thing right! 
//--------------------------------------------------------------------


//--------------------------------------------------------------------
static void EmptyJob( void *ptr )
{
   uint *count_ptr = (uint*)ptr;
   AtomicIncrement( count_ptr );
}

//--------------------------------------------------------------------
static bool gDone = false;
static void OnEverythingDone( void *ptr )
{
   uint *count_ptr = (uint*)ptr;

   // assert count_ptr is 1000 [make sure all other jobs got to run first!]
   uint count = *count_ptr; 
   if (count != 1000) {
      __debugbreak();
   }

   gDone = true;
}

//--------------------------------------------------------------------
void JobSystemTest()
{
   JobSystemStartup( JOB_TYPE_COUNT );


   // Make sure creating and releasing work [does not dispatch - job does not run!]
   Job *job = JobCreate( JOB_GENERIC, EmptyJob, nullptr );
   JobRelease( job );

   // Next, lets kick off jobs, and make sure they're freeing up.
   {
      PROFILE_LOG_SCOPE("JobDispatchAndReleaseTime");
      uint count = 0;
      Job *final_job = JobCreate( JOB_GENERIC, OnEverythingDone, &count );

      for (uint i = 0; i < 1000; ++i) {
         job = JobCreate( JOB_GENERIC, EmptyJob, &count );
         final_job->dependent_on( job );
         JobDispatchAndRelease( job );
      }

      // now I can dispatch myself [if I do it first, it has no dependancies and will immediately run]
      JobDispatch( final_job );

      // wait for it to finish
      JobWaitAndRelease( final_job );

      // And after this is done, I should KNOW the count is 1000 [1001 jobs ran]
      if (count != 1000) {
         __debugbreak();
      }
   }

   JobSystemShutdown();
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

