#include "atto.h"
#include "profiling.h"


int wmain(int argc, const wchar_t * argv[])
{
	attoFile_reset(&file);

	// Initialise profiler, if applicable
	initProfiler();

	const wchar_t * fileName = atto_getFileName(argc, argv);
	if (fileName == NULL)
	{
		atto_printHelp(argv[0]);
		return 1;
	}

	if (!attoFile_open(&file, fileName, false))
	{
		atto_printErr(attoE_file);
		return 2;
	}
	attoFile_close(&file);

	// Set console title
	attoFile_setConTitle(&file);

	if (!attoData_init(&editor))
	{
		atto_printErr(attoE_window);
		return 3;
	}

	const wchar_t * res;
	writeProfiler("wmain", "Starting to read file...");
	if ((res = attoFile_read(&file)) != NULL)
	{
		attoData_statusDraw(&editor, res);
	}

	attoData_refresh(&editor);
	while (atto_loop());

	return 0;
}
