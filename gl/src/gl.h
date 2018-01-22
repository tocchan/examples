#pragma once


#include <stdarg.h>
#include "window.h"

#include "external/gl/glcorearb.h"
#include "external/gl/glext.h"
#include "external/gl/wglext.h"

#pragma comment( lib, "Opengl32.lib" )


//------------------------------------------------------------------------
// Defines Functions
//------------------------------------------------------------------------

// wgl - windows gl

// these are needed to create the actual context
extern PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;
extern PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
extern PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

// gl - common

// First, we will want to clear the screen; 
extern PFNGLCLEARCOLORPROC glClearColor; 
extern PFNGLCLEARPROC glClear; 

//------------------------------------------------------------------------
//------------------------------------------------------------------------
bool GLStartup( HWND hwnd ); 
void GLShutdown(); 

//------------------------------------------------------------------------
void GLPresent();

void GLCheckError(); 
