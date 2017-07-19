#pragma once
#if !defined( __JOB__ )
#define __JOB__

/************************************************************************/
/*                                                                      */
/* INCLUDE                                                              */
/*                                                                      */
/************************************************************************/
#include "common.h"

#include "ts_queue.h"
#include "signal.h"
#include "atomic.h"

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
enum eJobType : uint
{
   JOB_GENERIC = 0,
   JOB_MAIN, 
   JOB_IO, 
   JOB_RENDER, 

   JOB_TYPE_COUNT,
};

enum eJobState : uint
{
   JOB_STATE_WAITING,   // job that exists, and has dependencies [newly created jobs start in this state]
   JOB_STATE_ENQUEUED,  // job has been put into a queue.
   JOB_STATE_RUNNING,   // job has been picked up by a consumer is in the process of being run
   JOB_STATE_FINISHED,  // job has completed - JobWait spins until this is present
};

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
class Job;

typedef void (*job_work_cb)( void* );
typedef ThreadSafeQueue<Job*> JobQueue;

//--------------------------------------------------------------------
//--------------------------------------------------------------------
class Job
{
   public:
      // The queue I add myself too upon reading a dependancy count of 0
      eJobType type; 
      eJobState state;

      // function associated with this job
      job_work_cb work_cb;

      // data associated with this job
      void *user_data;

      // how many calls to dispatch are required before enqueuing?
      uint dependancy_count;

      // how many releases are required before deleting?
      uint ref_count;

      std::vector<Job*> dependents;
      
   public:
      void on_finish();
      void on_dependancy_finished(); 

      void dependent_on( Job *parent );

      inline bool is_finished() const { return state == JOB_STATE_FINISHED; }

   public:
      // used internally
      void set_state( eJobState new_state );
};

//--------------------------------------------------------------------
//--------------------------------------------------------------------
class JobConsumer
{
   public:
      void add_category( uint category );

      bool consume_job();
      uint consume_all_jobs();
      uint consume_for_ms( uint ms );

   public:
      std::vector<JobQueue*> queues;
};

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
void JobSystemStartup( uint job_category_count, int generic_thread_count = -1 );
void JobSystemShutdown();

void JobSystemSetSignal( uint category, Signal *signal );
JobQueue* JobSystemGetQueue( uint category );

// Creating a job starts with a depedency and reference [the caller]
Job* JobCreate( eJobType type, job_work_cb work_cb, void *user_data );

// Dispatch will attempt to run the job by decrementing the dependency count
// If the dependency count reaches 0, the job is enqueued and allowed to be
// picked up by a consumer.  If it is not 0, the function does nothing else 
// [it is still dependent on someone]
void JobDispatch( Job *job );

// Job release removes a single reference to the object.  Upon reaching 0 references
// the job is freed up.
void JobAcquire( Job *job );
void JobRelease( Job *job );

// Exactly as the name implies.
// Equivalent of a ThreadDetach for jobs.
void JobDispatchAndRelease( Job *job );

// Wait on a job.  If a consumer is passed in
// we'll consume jobs while we wait using that consumer;
// (another way to do this is associate a consumer with a thread id
// and use that)
void JobWait( Job *job, JobConsumer *consumer = nullptr );

// equivalent of a ThreadJoin for this system.
void JobWaitAndRelease( Job *job, JobConsumer *consumer = nullptr );



void JobSystemTest();


#endif 
