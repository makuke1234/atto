#include "pico.h"

#include <stdlib.h>
#include <string.h>

pico_File file;
pico_DS editor;

int wmain(int argc, const wchar_t ** argv)
{
	// Set console to Unicode mode
	const wchar_t * fileName = pico_getFileName(argc, argv);
	if (fileName == NULL)
	{
		pico_printHelp(argv[0]);
		return 1;
	}
	if (picoFile_open(&file, fileName) == false)
	{
		pico_printErr(picoE_file);
		return 2;
	}
	// Set console title
	picoFile_setConTitle(&file);

	if (!picoDS_init(&editor))
	{
		pico_printErr(picoE_window);
		return 3;
	}

	const wchar_t * res;
	if ((res = picoFile_read(&file)) != NULL)
	{
		picoDS_statusDraw(&editor, res);
	}

	picoDS_refresh(&editor);
	while (pico_loop());

	return 0;
}

bool boolarrGet(uint8_t * arr, const size_t index)
{
	return (arr[index / 8] >> (index % 8)) & 0x01;
}
void boolarrPut(uint8_t * arr, const size_t index, const bool value)
{
	const size_t in1 = index / 8;
	const uint8_t pattern = 0x01 << (index - (8 * in1));
	(value) ? (arr[in1] |= pattern) : (arr[in1] &= (uint8_t)~pattern);
}

bool picoFile_open(pico_File * restrict file, const wchar_t * restrict fileName)
{
	// try to open file
	file->hFile = CreateFileW(
		fileName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (file->hFile == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_SHARING_VIOLATION)
		{
			file->hFile = CreateFileW(
				fileName,
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL
			);
			if (file->hFile == INVALID_HANDLE_VALUE)
			{
				file->canWrite = false;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	else
	{
		file->canWrite = true;
	}

	file->fileName = fileName;
	return true;
}
const wchar_t * picoFile_read(pico_File * restrict file)
{
	// Read file contents
	if (file->hFile == INVALID_HANDLE_VALUE)
	{
		return L"File handle error!";
	}

	// Get file size
	DWORD fileSize = GetFileSize(file->hFile, NULL);
	// Alloc array
	char * bytes = malloc(fileSize * sizeof(char));
	if (bytes == NULL)
	{
		return L"Memory error!";
	}
	if (!ReadFile(
			file->hFile,
			bytes,
			fileSize,
			NULL,
			NULL
		)
	)
	{
		free(bytes);
		return L"File read error!";
	}
	// Move file pointer back to beginning
	SetFilePointer(file->hFile, 0, NULL, FILE_BEGIN);

	// Convert to UTF-16
	wchar_t * utf16;
	uint32_t chars = pico_convToUnicode(bytes, &utf16);
	if (utf16 == NULL)
	{
		free(bytes);
		return L"Unicode conversion error!";
	}
	// Free loaded file memory
	free(bytes);

	// Save lines to structure
	wchar_t ** lines;
	uint32_t numLines = pico_strnToLines(utf16, chars, &lines);
	if (lines == NULL)
	{
		free(utf16);
		return L"Line reading error!";
	}

	for (uint32_t i = 0; i < numLines; ++i)
	{
		
	}

	// Free lines arr
	free(lines);
	// Free UTF-16 converted string
	free(utf16);

	return NULL;
}
int picoFile_write(pico_File * restrict file)
{
	return -1;
}
void picoFile_setConTitle(pico_File * restrict file)
{
	wchar_t wndName[MAX_PATH];
	size_t fnamelen = wcslen(file->fileName);
	memcpy(wndName, file->fileName, fnamelen * sizeof(wchar_t));
	wcscpy_s(wndName + fnamelen, MAX_PATH - fnamelen, L" - pico");
	SetConsoleTitleW(wndName);
}


bool picoFile_addNormalCh(pico_File * restrict file, wchar_t ch)
{

}
bool picoFile_addSpecialCh(pico_File * restrict file, wchar_t ch)
{
	switch (ch)
	{
	case VK_TAB:
		for (int i = 0; i < 4; ++i) picoFile_addNormalCh(file, ' ');
		break;
	case VK_OEM_BACKTAB:
		// Check if there's 4 spaces before the caret
		if (picoFile_checkLineAt(file, -4, L"    ", 4))
		{
			for (int i = 0; i < 4; ++i) picoFile_deleteForward(file);
		}
		// If there isn't, check if there's 4 spaces after the caret
		else if (picoFile_checkLineAt(file, 0, L"    ", 4))
		{
			for (int i = 0; i < 4; ++i) picoFile_deleteBackward(file);
		}
		break;
	case VK_RETURN:	// Enter key
		picoFile_addNewLine(file);
	case VK_BACK:	// Backspace
		picoFile_deleteBackward(file);
	case VK_DELETE:	// Delete
		picoFile_deleteForward(file);
	case VK_LEFT:	// Left arrow
	case VK_RIGHT:	// Right arrow
	case VK_UP:		// Up arrow
	case VK_DOWN:	// Down arrow
	default:
		return false;
	}
	return true;
}

bool picoFile_checkLineAt(pico_File * restrict file, int32_t maxdelta, const wchar_t * string, uint32_t maxString)
{

}
bool picoFile_deleteForward(pico_File * restrict file)
{

}
bool picoFile_deleteBackward(pico_File * restrict file)
{

}
bool picoFile_addNewLine(pico_File * restrict file)
{

}


void picoFile_destruct(pico_File * restrict file)
{
	if (file->fileName != NULL && file->hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file->hFile);
		file->hFile = INVALID_HANDLE_VALUE;
	}
}


bool picoDS_init(pico_DS * restrict ds)
{
	ds->conIn  = GetStdHandle(STD_INPUT_HANDLE);
	ds->conOut = GetStdHandle(STD_OUTPUT_HANDLE);
	// Set exit handler
	atexit(&pico_exitHandler);

	// Get console current size
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(ds->conOut, &csbi))
		return false;
	
	ds->scrbuf.linew = (uint32_t)(csbi.srWindow.Right  - csbi.srWindow.Left + 1);
	ds->scrbuf.lines = (uint32_t)(csbi.srWindow.Bottom - csbi.srWindow.Top  + 1);
	// Create screen buffer
	ds->scrbuf.handle = CreateConsoleScreenBuffer(
		GENERIC_WRITE,
		0,
		NULL,
		CONSOLE_TEXTMODE_BUFFER,
		NULL
	);
	if (ds->scrbuf.handle == INVALID_HANDLE_VALUE)
		return false;
	
	ds->scrbuf.mem = malloc((size_t)(ds->scrbuf.linew * ds->scrbuf.lines) * sizeof(wchar_t));
	if (ds->scrbuf.mem == NULL)
		return false;
	
	for (uint32_t i = 0, sz = ds->scrbuf.linew * ds->scrbuf.lines; i < sz; ++i)
	{
		ds->scrbuf.mem[i] = L' ';
	}
	if (!SetConsoleScreenBufferSize(ds->scrbuf.handle, (COORD){ .X = (SHORT)ds->scrbuf.linew, .Y = (SHORT)ds->scrbuf.lines }))
		return false;
	if (!SetConsoleActiveScreenBuffer(ds->scrbuf.handle))
		return false;

	return true;
}
void picoDS_refresh(pico_DS * restrict ds)
{
	pico_updateScrbuf();
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		ds->scrbuf.handle,
		ds->scrbuf.mem,
		ds->scrbuf.linew * (ds->scrbuf.lines - 1),
		(COORD){ 0, 0 },
		&dwBytes
	);
}
void picoDS_refreshAll(pico_DS * restrict ds)
{
	pico_updateScrbuf();
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		ds->scrbuf.handle,
		ds->scrbuf.mem,
		ds->scrbuf.linew * ds->scrbuf.lines,
		(COORD){ 0, 0 },
		&dwBytes
	);
}
void picoDS_statusDraw(pico_DS * restrict ds, const wchar_t * message)
{
	size_t len = wcslen(message), effLen = (len > ds->scrbuf.linew) ? (size_t)ds->scrbuf.linew : len;
	wchar_t * restrict lastLine = ds->scrbuf.mem + (ds->scrbuf.lines - 1) * ds->scrbuf.linew;
	memcpy(
		lastLine,
		message,
		sizeof(wchar_t) * effLen
	);
	for (size_t i = effLen; i < ds->scrbuf.linew; ++i)
	{
		lastLine[i] = L' ';
	}
	picoDS_statusRefresh(ds);
}
void picoDS_statusRefresh(pico_DS * restrict ds)
{
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		ds->scrbuf.handle,
		ds->scrbuf.mem + (ds->scrbuf.lines - 1) * ds->scrbuf.linew,
		ds->scrbuf.linew,
		(COORD){ .X = 0, .Y = (SHORT)(ds->scrbuf.lines - 1) },
		&dwBytes
	);
}

void picoDS_destruct(pico_DS * restrict ds)
{
	if (ds->scrbuf.mem)
	{
		free(ds->scrbuf.mem);
		ds->scrbuf.mem = NULL;
	}
	if (ds->scrbuf.handle)
	{
		SetConsoleActiveScreenBuffer(ds->conOut);
	}
}


void pico_exitHandler()
{
	// Clear resources
	picoFile_destruct(&file);
	picoDS_destruct(&editor);
}

const wchar_t * pico_getFileName(const int argc, const wchar_t * const * const argv)
{
	return (argc > 1) ? argv[1] : NULL;
}
void pico_printHelp(const wchar_t * app)
{
	puts("Correct usage:");
	fputws(app, stdout);
	puts(" [file]");
}

static const char * pico_errCodes[picoE_num_of_elems] = {
	"Uknown error occurred!",
	"Error reading file!",
	"Error initialising window!"
};
void pico_printErr(enum picoE errCode)
{
	if (errCode >= picoE_num_of_elems)
		errCode = picoE_unknown;
	
	puts(pico_errCodes[errCode]);
}

bool pico_loop()
{
	enum SpecialAsciiCodes
	{
		sac_Ctrl_Q = 17,
		sac_Ctrl_S = 19,

		sac_last_code = 31
	};

	INPUT_RECORD ir;
	DWORD evRead;
	ReadConsoleInputW(editor.conIn, &ir, 1, &evRead);
	if (evRead != 0 && ir.EventType == KEY_EVENT)
	{
		static uint8_t keybuffer[32] = { 0 }, prevkeybuffer[32] = { 0 };
		static wchar_t prevkey;
		static bool prevstate;

		static int keyCount = 1;


		wchar_t key = ir.Event.KeyEvent.uChar.UnicodeChar;
		wchar_t wVirtKey = ir.Event.KeyEvent.wVirtualKeyCode;
		bool state = ir.Event.KeyEvent.bKeyDown != 0;

		if (state == true)
		{
			if (key == prevkey)
			{
				++keyCount;
			}
			else
			{
				keyCount = 1;
			}
		}


		if (state)
		{
			boolarrPut(keybuffer, key, true);
			if (wVirtKey == VK_ESCAPE || key == sac_Ctrl_Q)	// Exit on Escape or Ctrl+Q
				return false;
			else if (boolarrGet(keybuffer, sac_Ctrl_S) && !boolarrGet(prevkeybuffer, sac_Ctrl_S))	// Save file
			{
				int saved = picoFile_write(&file);
				if (saved == -1)
				{
					picoDS_statusDraw(&editor, L"Nothing new to save");
				}
				else
				{
					wchar_t tempstr[MAX_STATUS];
					swprintf_s(tempstr, MAX_STATUS, L"Wrote %d bytes.", saved);
					picoDS_statusDraw(&editor, tempstr);
				}
			}
			// Normal keys
			else if (key > sac_last_code)
			{
				wchar_t tempstr[MAX_STATUS];
				swprintf_s(tempstr, MAX_STATUS, L"'%c' #%d", key, keyCount);
				picoDS_statusDraw(&editor, tempstr);
				if (picoFile_addNormalCh(&file, key))
				{
					picoDS_refresh(&editor);
				}
			}
			// Special keys
			else
			{
				switch (wVirtKey)
				{
				case VK_TAB:
					if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
					{
						picoDS_statusDraw(&editor, L"\u2191 + 'TAB'");
						wVirtKey = VK_OEM_BACKTAB;
					}
					else
					{
						picoDS_statusDraw(&editor, L"'TAB'");
					}
					break;
				case VK_RETURN:	// Enter key
				case VK_BACK:	// Backspace
				case VK_DELETE:	// Delete
				case VK_LEFT:	// Left arrow
				case VK_RIGHT:	// Right arrow
				case VK_UP:		// Up arrow
				case VK_DOWN:	// Down arrow
				{
					static const wchar_t * buf[] = {
						[VK_RETURN] = L"'RET'",
						[VK_BACK]   = L"'BS'",
						[VK_DELETE] = L"'DEL'",
						[VK_LEFT]   = L"\u2190",
						[VK_RIGHT]  = L"\u2192",
						[VK_UP]     = L"\u25b2",
						[VK_DOWN]   = L"\u25bc"
					};
					picoDS_statusDraw(&editor, buf[wVirtKey]);
					break;
				}
				case VK_CAPITAL:
				{
					wchar_t tempstr[MAX_STATUS];
					swprintf_s(tempstr, MAX_STATUS, L"'CAPS' %s", (GetKeyState(VK_CAPITAL) & 0x0001) ? L"On" : L"Off");
					picoDS_statusDraw(&editor, tempstr);
					break;
				}
				case VK_NUMLOCK:
				{
					wchar_t tempstr[MAX_STATUS];
					swprintf_s(tempstr, MAX_STATUS, L"'NUMLOCK' %s", (GetKeyState(VK_NUMLOCK) & 0x0001) ? L"On" : L"Off");
					picoDS_statusDraw(&editor, tempstr);
					break;
				}
				case VK_SCROLL:
				{
					wchar_t tempstr[MAX_STATUS];
					swprintf_s(tempstr, MAX_STATUS, L"'SCRLOCK' %s", (GetKeyState(VK_SCROLL) & 0x0001) ? L"On" : L"Off");
					picoDS_statusDraw(&editor, tempstr);
					break;
				}
				}
				if (picoFile_addSpecialCh(&file, wVirtKey))
				{
					picoDS_refresh(&editor);
				}
			}
		}
		else
		{
			boolarrPut(keybuffer, key, false);
		}
		prevkey = key;
		prevstate = state;
		memcpy(prevkeybuffer, keybuffer, 32 * sizeof(uint8_t));
		FlushConsoleInputBuffer(editor.conIn);
	}

	return true;
}
void pico_updateScrbuf()
{

}

uint32_t pico_convToUnicode(const char * utf8, wchar_t ** putf16)
{
	// Query the needed size
	int size = MultiByteToWideChar(
		CP_UTF8,
		MB_PRECOMPOSED,
		utf8,
		-1,
		NULL,
		0
	);
	// Try to allocate memory
	*putf16 = malloc((uint32_t)size * sizeof(wchar_t));
	if (*putf16 == NULL)
	{
		return 0;
	}
	// Make conversion
	MultiByteToWideChar(
		CP_UTF8,
		MB_PRECOMPOSED,
		utf8,
		-1,
		*putf16,
		size
	);
	return (uint32_t)size;
}
uint32_t pico_convFromUnicode(const wchar_t * utf16, char ** putf8)
{
	// Quory the needed size
	int size = WideCharToMultiByte(
		CP_UTF8,
		MB_PRECOMPOSED,
		utf16,
		-1,
		NULL,
		0,
		NULL,
		NULL
	);
	// Alloc mem
	*putf8 = malloc((uint32_t)size * sizeof(char));
	if (*putf8 == NULL)
	{
		return 0;
	}
	// Convert
	WideCharToMultiByte(
		CP_UTF8,
		MB_PRECOMPOSED,
		utf16,
		-1,
		*putf8,
		size,
		NULL,
		NULL
	);
	return (uint32_t)size;
}
uint32_t pico_strnToLines(wchar_t * utf16, uint32_t chars, wchar_t *** lines)
{
	// Count number of newline characters
	uint32_t newlines = 1;
	for (uint32_t i = 0; i < chars; ++i)
	{
		if (utf16[i] == '\n')
			++newlines;
	}
	*lines = malloc(newlines * sizeof(wchar_t *));
	if (*lines == NULL)
	{
		return 0;
	}

	uint32_t starti = 0, j = 0;
	for (uint32_t i = 0; i < chars; ++i)
	{
		if (utf16[i] == '\n')
		{
			utf16[i] = '\0';
			(*lines)[j] = &utf16[starti];
			starti = i + 1;
			++j;
		}
	}
	(*lines)[j] = &utf16[starti];

	return newlines;
}
