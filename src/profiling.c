#include "profiling.h"

#if PROFILING_ENABLE == 1

#include <time.h>
#include <stdarg.h>

static FILE * profilingFile = NULL;

void initProfiler(void)
{
	profilingFile = fopen("profiler.txt", "a+");
	if (profilingFile == NULL)
	{
		fputs("Error opening profiling file!\n", stderr);
		exit(1);
	}
	fputs("\n", profilingFile);
	writeProfiler("initProfiler", "Started application...");
}
void writeProfiler(const char * restrict function, const char * restrict format, ...)
{
	if (profilingFile == NULL)
	{
		fputs("Profiling file not open!\n", stderr);
		return;
	}

	// Write timestamp
	time_t rawtime;
	time(&rawtime);
	struct tm * ti = localtime(&rawtime);
	fprintf(
		profilingFile,
		"[%.2d.%.2d.%d @%.2d:%.2d:%.2d] @%s<",
		ti->tm_mday, ti->tm_mon + 1, ti->tm_year + 1900,
		ti->tm_hour, ti->tm_min,     ti->tm_sec,
		function
	);
	// Write message
	va_list ap;
	va_start(ap, format);

	vfprintf(profilingFile, format, ap);

	va_end(ap);

	fprintf(profilingFile, ">\n");
	fflush(profilingFile);
}

#define PROFILER_STACK_SIZE 256

static clock_t profilerStack[PROFILER_STACK_SIZE];
static int curStackLen = 0;

void profilerStart(void)
{
	if (curStackLen < PROFILER_STACK_SIZE)
	{
		profilerStack[curStackLen++] = clock();
	}
}
void profilerEnd(const char * funcName)
{
	if (curStackLen == 0)
	{
		return;
	}
	--curStackLen;
	writeProfiler(funcName, "Elapsed %.3f s", (double)(clock() - profilerStack[curStackLen]) / (double)CLOCKS_PER_SEC);
}

#endif
