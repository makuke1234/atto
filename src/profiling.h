#ifndef PROFILING_H
#define PROFILING_H

#if PROFILING_ENABLE == 1

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

void initProfiler();
void writeProfiler(const char * function, const char * format, ...);
void profilerStart();
void profilerEnd(const char * funcName);


#else

#define initProfiler()
#define writeProfiler(function, format, ...)
#define profilerStart()
#define profilerEnd(funcName)

#endif
#endif
