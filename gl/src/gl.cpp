#include "gl.h"
#include <stdio.h>

#include "log.h"

typedef unsigned int uint;


//------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------

// our rendering context; 
static HMODULE gGLLibrary = NULL; 
static HWND gGLwnd = NULL;     // window our context is attached to; 
static HDC gHDC = NULL;          // our device context
static HGLRC gGLContext = NULL;  // our rendering context; 

//------------------------------------------------------------------------
// wgl
// Render/glfunctions.cpp
PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB = nullptr;
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;

// gl
PFNGLCLEARCOLORPROC glClearColor = nullptr; 
PFNGLCLEARPROC glClear = nullptr; 



//------------------------------------------------------------------------
// LOCAL FUNCTIONS & MACROS
//------------------------------------------------------------------------

// Use this to deduce type of the pointer so we can cast; 
template <typename T>
bool wglGetTypedProcAddress( T *out, char const *name ) 
{
   // Grab the function from the currently bound render contect
   // most opengl 2.0+ features will be found here
   *out = (T) wglGetProcAddress(name); 

   if ((*out) == nullptr) {
      // if it is not part of wgl (the device), then attempt to get it from the GLL library
      // (most OpenGL functions come from here)
      *out = (T) GetProcAddress( gGLLibrary, name ); 
   }

   return (*out != nullptr);

   wglGetTypedProcAddress( &glClear, "glClear" ); 
}

// Use a macro for the compile-time string creation; 
#define GL_BIND_FUNCTION(f)      wglGetTypedProcAddress( &f, #f )



//------------------------------------------------------------------------
// C Functions
//------------------------------------------------------------------------

//------------------------------------------------------------------------
static void BindGLFunctions()
{
   GL_BIND_FUNCTION( glClearColor ); 
   GL_BIND_FUNCTION( glClear ); 
}

//------------------------------------------------------------------------
static HGLRC CreateOldRenderContext( HDC hdc ) 
{
   // Setup the output to be able to render how we want
   // (in our case, an RGBA (4 bytes per channel) output that supports OpenGL
   // and is double buffered
   PIXELFORMATDESCRIPTOR pfd;
   memset( &pfd, 0, sizeof(pfd) ); 
   pfd.nSize = sizeof(pfd);
   pfd.nVersion = 1;
   pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.cColorBits = 32;
   pfd.cDepthBits = 0; // 24; Depth/Stencil handled by FBO
   pfd.cStencilBits = 0; // 8; DepthStencil handled by FBO
   pfd.iLayerType = PFD_MAIN_PLANE; // ignored now according to MSDN

   // Find a pixel format that matches our search criteria above. 
   int pixel_format = ::ChoosePixelFormat( hdc, &pfd );
   if ( pixel_format == NULL ) {
      return NULL; 
   }

   // Set our HDC to have this output. 
   if (!::SetPixelFormat( hdc, pixel_format, &pfd )) {
      return NULL; 
   }

   // Create the context for the HDC
   HGLRC context = wglCreateContext( hdc );
   if (context == NULL) {
      return NULL; 
   }

   // return the context; 
   return context; 
}

//------------------------------------------------------------------------
// Creates a real context as a specific version (major.minor)
static HGLRC CreateRealRenderContext( HDC hdc, int major, int minor ) 
{
   // So similar to creating the temp one - we want to define 
   // the style of surface we want to draw to.  But now, to support
   // extensions, it takes key_value pairs
   int const format_attribs[] = {
      WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,    // The rc will be used to draw to a window
      WGL_SUPPORT_OPENGL_ARB, GL_TRUE,    // ...can be drawn to by GL
      WGL_DOUBLE_BUFFER_ARB, GL_TRUE,     // ...is double buffered
      WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB, // ...uses a RGBA texture
      WGL_COLOR_BITS_ARB, 24,             // 24 bits for color (8 bits per channel)
      // WGL_DEPTH_BITS_ARB, 24,          // if you wanted depth a default depth buffer...
      // WGL_STENCIL_BITS_ARB, 8,         // ...you could set these to get a 24/8 Depth/Stencil.
      NULL, NULL,                         // Tell it we're done.
   };

   // Given the above criteria, we're going to search for formats
   // our device supports that give us it.  I'm allowing 128 max returns (which is overkill)
   size_t const MAX_PIXEL_FORMATS = 128;
   int formats[MAX_PIXEL_FORMATS];
   int pixel_format = 0;
   UINT format_count = 0;

   BOOL succeeded = wglChoosePixelFormatARB( hdc, format_attribs, nullptr, MAX_PIXEL_FORMATS, formats, (UINT*)&format_count );
   if (!succeeded) {
      return NULL; 
   }

   // Loop through returned formats, till we find one that works
   for (uint i = 0; i < format_count; ++i) {
      pixel_format = formats[i];
      succeeded = SetPixelFormat( hdc, pixel_format, NULL ); // same as the temp context; 
      if (succeeded) {
         break;
      } else {
         DWORD error = GetLastError();
         Logf( "Failed to set the format: %u", error ); 
      }
   }

   if (!succeeded) {
      return NULL; 
   }

   // Okay, HDC is setup to the rihgt format, now create our GL context

   // First, options for creating a debug context (potentially slower, but 
   // driver may report more useful errors). 
   int context_flags = 0; 
   #if defined(_DEBUG)
      context_flags |= WGL_CONTEXT_DEBUG_BIT_ARB; 
   #endif

   // describe the context
   int const attribs[] = {
      WGL_CONTEXT_MAJOR_VERSION_ARB, major,                             // Major GL Version
      WGL_CONTEXT_MINOR_VERSION_ARB, minor,                             // Minor GL Version
      WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,   // Restrict to core (no compatibility)
      WGL_CONTEXT_FLAGS_ARB, context_flags,                             // Misc flags (used for debug above)
      0, 0
   };

   // Try to create context
   HGLRC context = wglCreateContextAttribsARB( hdc, NULL, attribs );
   if (context == NULL) {
      return NULL; 
   }

   return context;
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
bool GLStartup( HWND hwnd )
{
   // bind the dll - some functions will come from here; 
   gGLLibrary = ::LoadLibraryA( "opengl32.dll" ); 
   if (gGLLibrary == NULL) {
      return false; 
   }

   HDC hdc = ::GetDC( hwnd );       // Get the Device Context (DC) - how windows handles the interace to rendering devices
                                    // This "acquires" the resource - to cleanup, you must have a ReleaseDC(hwnd, hdc) call. 

   // We first create the fake context so we can fetch the more modern function needed to create the "real" context. 
   HGLRC temp_context = CreateOldRenderContext( hdc ); // use the DC to create a rendering context (handle for all OpenGL state - like a pointer)
   if (temp_context == NULL) {
      // assert
      ::ReleaseDC(hwnd, hdc); 
      return false; 
   }

   wglMakeCurrent( hdc, temp_context ); 

   // this is the actual code for fetching a function from a DLL, or more particularly, a function for our
   // particular device. 
   wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC) wglGetProcAddress( "wglGetExtensionsStringARB" ); 

   // These do the same as the above line, but using macros/templates to make the code a little cleaner;
   GL_BIND_FUNCTION( wglChoosePixelFormatARB ); 
   GL_BIND_FUNCTION( wglCreateContextAttribsARB );

   // Create, now we have the core creation functions I need, so I can create a real context; 
   HGLRC real_context = CreateRealRenderContext( hdc, 4, 2 ); 
   wglDeleteContext(temp_context); 

   // setting the globals
   if (real_context != NULL) {

      wglMakeCurrent( hdc, real_context ); 
      BindGLFunctions(); 

      gHDC = hdc; 
      gGLContext = real_context; 
      gGLwnd = hwnd; 
   } else {
      ::ReleaseDC( hwnd, hdc ); 
   }

   return gGLContext != NULL; 
}

void BindNewWGLFunctions()
{
   GL_BIND_FUNCTION( wglGetExtensionsStringARB ); 
   GL_BIND_FUNCTION( wglChoosePixelFormatARB ); 
   GL_BIND_FUNCTION( wglCreateContextAttribsARB );
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void GLShutdown()
{
   wglMakeCurrent( gHDC, NULL ); 

   ::wglDeleteContext( gGLContext ); 
   ::ReleaseDC( gGLwnd, gHDC ); 

   gGLContext = NULL; 
   gHDC = NULL;
   gGLwnd = NULL; 

   ::FreeLibrary( gGLLibrary ); 
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void GLPresent()
{
   // flip the back buffer to the front (copy it to the window)
   ::SwapBuffers( gHDC ); 
}

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
void GLCheckError()
{
   // checking for errors is slow - since GPUs are naturally parallel, if we
   // poll for error, we hvae to let the GPU finish what it is doing.  
   // BUT, it is extremely useful for debugging to know exactly which line caused the error;

   // So I do this on a DEBUG preprocessor flag.  In release, this entire function will 
   // get compiled out. 
   #if defined(_DEBUG)
   #endif
}
