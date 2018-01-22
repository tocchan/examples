#include "demo.h"

#include "window.h"
#include "gl.h"

// D3D Stuff

//------------------------------------------------------------------------
// DEFINES
//------------------------------------------------------------------------
#define WINDOW_TITLE    "D3D11 SETUP"
#define WINDOW_RES_X    (1280)
#define WINDOW_RES_Y    (720)

#define RENDER_DEBUG

typedef unsigned int uint;

//------------------------------------------------------------------------
// MACROS
//------------------------------------------------------------------------
#define SAFE_DELETE(ptr)   if ((ptr) != nullptr) { delete ptr; ptr = nullptr }



//------------------------------------------------------------------------
// Declare functions this demo uses
//------------------------------------------------------------------------
void DemoRender();            // Does rendering for this demo


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void DemoRender()
{
   glClearColor( .2f, .2f, .4f, 1.0f ); 
   glClear( GL_COLOR_BUFFER_BIT ); 


   GLPresent(); 
}

//------------------------------------------------------------------------
// DEMO ENTRY POINT
//------------------------------------------------------------------------
void DemoRun()
{
   HWND hwnd = CreateTheWindow( "GL DEMO", 50, 50, 1280, 720 ); 
   GLStartup( hwnd ); 

   // While this window is open, process messages, and do other game processing
   while (WindowIsOpen(hwnd)) {
      ProcessWindowMessages(hwnd);

      // Do other stuff.
      DemoRender();
   }
  
   // cleanup
   GLShutdown(); 
   ::DestroyWindow(hwnd);
}
