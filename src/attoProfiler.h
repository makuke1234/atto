#ifndef PROFILING_H
#define PROFILING_H

#if PROFILING_ENABLE == 1

#include "common.h"

/**
 * @brief Initialise profiler, exits on failure
 * 
 */
void initProfiler(void);
/**
 * @brief Write a profiler log message
 * 
 * @param function Function name of writer
 * @param format Standard printf message format
 * @param ... Variadic format arguments
 */
void writeProfiler(const char * restrict function, const char * restrict format, ...);
/**
 * @brief Start profiler timestamp
 * 
 */
void profilerStart(void);
/**
 * @brief Stop profiler timestamp
 * 
 * @param funcName Function name of timestamp writer
 */
void profilerEnd(const char * funcName);


#else

#define initProfiler()
#define writeProfiler(function, ...)
#define profilerStart()
#define profilerEnd(funcName)

typedef int make_iso_compilers_happy;

#endif
#endif
