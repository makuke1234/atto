#include "atto.h"

static attoData_t editor;

int wmain(int argc, const wchar_t * argv[])
{
	atto_exitHandlerSetVars(&editor);
	attoData_reset(&editor);

	// Initialise profiler, if applicable
	initProfiler();

	const wchar_t * fileName = atto_getFileName(argc, argv);
	if (fileName == NULL)
	{
		atto_printHelp(argv[0]);
		return 1;
	}

	if (!attoFile_open(&editor.file, fileName, false))
	{
		atto_printErr(attoE_file);
		return 2;
	}
	attoFile_close(&editor.file);

	// Set console title
	attoFile_setConTitle(&editor.file);

	if (!attoData_init(&editor))
	{
		atto_printErr(attoE_window);
		return 3;
	}

	const wchar_t * res;
	writeProfiler("wmain", "Starting to read file...");
	if ((res = attoFile_read(&editor.file)) != NULL)
	{
		attoData_statusDraw(&editor, res);
	}
	else
	{
		wchar_t tempstr[MAX_STATUS];
		swprintf_s(
			tempstr,
			MAX_STATUS,
			L"File loaded successfully! %s%s line endings.",
			(editor.file.eolSeq & EOL_CR) ? L"CR" : L"",
			(editor.file.eolSeq & EOL_LF) ? L"LF" : L""
		);
		attoData_statusDraw(&editor, tempstr);
	}

	attoData_refresh(&editor);
	while (atto_loop(&editor));

	return 0;
}
