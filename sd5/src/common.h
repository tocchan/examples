#pragma once

#include <atomic>
#include <stdint.h>
#include <stdio.h>
#include <conio.h>

typedef unsigned int uint;
typedef unsigned __int8 byte_t;

// used in a sample
extern std::atomic<int> gA;
extern int gB;


void pause();