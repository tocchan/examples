

#include "common.h"

#include <conio.h>
#include <stdio.h>

//----------------------------------------------------------------
// used in a sample
std::atomic<int> gA = 0;
int gB = 0;

//----------------------------------------------------------------
void pause()
{
   printf( "Press any key to continue..." );
   _getch();
   printf( "\n" );
}
