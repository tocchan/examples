#pragma once

/************************************************************************/
/*                                                                      */
/* INCLUDE                                                              */
/*                                                                      */
/************************************************************************/
#include "common.h"

/************************************************************************/
/*                                                                      */
/* DEFINES AND CONSTANTS                                                */
/*                                                                      */
/************************************************************************/
#define MAX_FRAMES_PER_CALLSTACK (128)

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
struct callstack_line_t 
{
   char filename[128];
   char function_name[128];
   uint32_t line;
   uint32_t offset;
};

/************************************************************************/
/*                                                                      */
/* CLASSES                                                              */
/*                                                                      */
/************************************************************************/
class Callstack
{
   public:
      Callstack(); 
   
      uint32_t hash;
      uint frame_count;
      void* frames[MAX_FRAMES_PER_CALLSTACK];
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

bool CallstackSystemInit();
void CallstackSystemDeinit();

// Feel free to reorg this in a way you like - this is very C style.  
// Really, we just want to make sure these callstacks are not allocated on the heap.
// - You could creat them in-place in the meta-data if you prefer (provide memory to fill)
// - You could overload new on the Callstack class, cause "new Callstack(skip_frames)" to call that, a
//   and keeping this functionality.

// As this is - this will create a callstack using malloc (untracked allocation), skipping the first few frames.
Callstack* CreateCallstack( uint skip_frames );
void DestroyCallstack( Callstack *c );

uint CallstackGetLines( callstack_line_t *line_buffer, uint const max_lines, Callstack *cs );

void CallstackDemo();

