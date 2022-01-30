#ifndef PROFILING_H
#define PROFILING_H

#if PROFILING_ENABLE == 1

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

void initProfiler(void);
void writeProfiler(const char * restrict function, const char * restrict format, ...);
void profilerStart(void);
void profilerEnd(const char * funcName);


#else

#define initProfiler()
#define writeProfiler(function, ...)
#define profilerStart()
#define profilerEnd(funcName)

typedef int make_iso_compilers_happy;

#endif
#endif
