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
      ThreadSafeQueue<Job*> *queues;
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
   while (gJobSystem->is_running) {
      signal->wait();
      JobConsumeAll( JOB_GENERIC );
   }

   JobConsumeAll( JOB_GENERIC );
}

//------------------------------------------------------------------------
void Job::on_finish()
{
   for (uint i = 0; i < dependents.size(); ++i) {
      dependents[i]->on_dependancy_finished();
   }
}

//------------------------------------------------------------------------
void Job::on_dependancy_finished() 
{
   JobDispatchAndRelease( this );
}

//------------------------------------------------------------------------
void Job::dependent_on( Job *parent ) 
{
   AtomicIncrement( &num_dependencies );
   parent->dependents.push_back( this );
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
}


//------------------------------------------------------------------------
Job* JobCreate( eJobType type, job_work_cb work_cb, void *user_data )
{
   Job *job = new Job();
   job->type = type;
   job->work_cb = work_cb;
   job->user_data = user_data;
   job->num_dependencies = 1;

   return job;
}

//------------------------------------------------------------------------
void JobDispatchAndRelease( Job *job )
{
   // if I'm not ready to run, don't. 
   uint dcount = AtomicDecrement( &job->num_dependencies );
   if (dcount != 0) {
      return; 
   }


   gJobSystem->queues[job->type].enqueue( job );
   Signal *signal = gJobSystem->signals[job->type];
   if (nullptr != signal) {
      signal->signal_all();
   }
}

//------------------------------------------------------------------------
// THIS SHOULD BE MOVED TO A JOB CONSUMER OBJECT!
uint JobConsumeAll( eJobType type )
{
   Job *job;
   uint processed_jobs = 0;

   ThreadSafeQueue<Job*> &queue = gJobSystem->queues[type];
   while (queue.dequeue(&job)) {
      job->work_cb( job->user_data );
      ++processed_jobs;

      job->on_finish();
      delete job;
   }

   return processed_jobs;
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
