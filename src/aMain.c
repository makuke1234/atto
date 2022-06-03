#include "atto.h"

static aData_t editor;

int wmain(int argc, const wchar * argv[])
{
	atto_exitHandlerSetVars(&editor);
	aData_reset(&editor);

	const wchar * fileName = atto_getFileName(argc, argv);
	if (fileName == NULL)
	{
		atto_printHelp(argv[0]);
		return 1;
	}

	if (!attoFile_open(&editor.file, fileName, false))
	{
		atto_printErr(aerrFILE);
		return 2;
	}
	attoFile_close(&editor.file);

	// Set console title
	attoFile_setConTitle(&editor.file);

	if (!aData_init(&editor))
	{
		atto_printErr(aerrWINDOW);
		return 3;
	}

	const wchar * res;
	if ((res = attoFile_read(&editor.file)) != NULL)
	{
		aData_statusDraw(&editor, res);
	}
	else
	{
		wchar tempstr[MAX_STATUS];
		swprintf_s(
			tempstr,
			MAX_STATUS,
			L"File loaded successfully! %s%s EOL sequences",
			(editor.file.eolSeq & eolCR) ? L"CR" : L"",
			(editor.file.eolSeq & eolLF) ? L"LF" : L""
		);
		aData_statusDraw(&editor, tempstr);
	}

	aData_refresh(&editor);
	while (atto_loop(&editor));

	return 0;
}
