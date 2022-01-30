#include "atto.h"
#include "profiling.h"

#include <stdlib.h>
#include <string.h>


attoFile file = { .hFile = INVALID_HANDLE_VALUE };
attoData editor = { 0 };

bool boolGet(uint8_t * arr, const size_t index)
{
	return (arr[index / 8] >> (index % 8)) & 0x01;
}
void boolPut(uint8_t * arr, const size_t index, const bool value)
{
	const size_t in1 = index / 8;
	const uint8_t pattern = 0x01 << (index - (8 * in1));
	(value) ? (arr[in1] |= pattern) : (arr[in1] &= (uint8_t)~pattern);
}

int32_t i32Min(int32_t a, int32_t b)
{
	return (a < b) ? a : b;
}
int32_t i32Max(int32_t a, int32_t b)
{
	return (a < b) ? b : a;
}
uint32_t u32Min(uint32_t a, uint32_t b)
{
	return (a < b) ? a : b;
}
uint32_t u32Max(uint32_t a, uint32_t b)
{
	return (a < b) ? b : a;
}



void atto_exitHandler()
{
	// Clear resources
	attoFile_destruct(&file);
	attoDS_destruct(&editor);
}

const wchar_t * atto_getFileName(const int argc, const wchar_t * const * const argv)
{
	return (argc > 1) ? argv[1] : NULL;
}
void atto_printHelp(const wchar_t * app)
{
	puts("Correct usage:");
	fputws(app, stdout);
	puts(" [file]");
}

static const char * atto_errCodes[attoE_num_of_elems] = {
	"Uknown error occurred!",
	"Error reading file!",
	"Error initialising window!"
};
void atto_printErr(enum attoErr errCode)
{
	if (errCode >= attoE_num_of_elems)
	{
		errCode = attoE_unknown;
	}
	puts(atto_errCodes[errCode]);
}

bool atto_loop()
{
	enum SpecialAsciiCodes
	{
		sac_Ctrl_Q = 17,
		sac_Ctrl_R = 18,
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
		//static bool prevstate;

		static int keyCount = 1;

		wchar_t key      = ir.Event.KeyEvent.uChar.UnicodeChar;
		wchar_t wVirtKey = ir.Event.KeyEvent.wVirtualKeyCode;
		bool state       = ir.Event.KeyEvent.bKeyDown != 0;

		if (state)
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
			boolPut(keybuffer, key, true);
			if (wVirtKey == VK_ESCAPE || key == sac_Ctrl_Q)	// Exit on Escape or Ctrl+Q
			{
				return false;
			}
			else if (boolGet(keybuffer, sac_Ctrl_R) && !boolGet(prevkeybuffer, sac_Ctrl_R))	// Reload file
			{
				const wchar_t * res;
				if ((res = attoFile_read(&file)) != NULL)
				{
					attoDS_statusDraw(&editor, res);
				}
				else
				{
					attoDS_statusDraw(&editor, L"File reloaded successfully!");
				}
				attoDS_refresh(&editor);
			}
			else if (boolGet(keybuffer, sac_Ctrl_S) && !boolGet(prevkeybuffer, sac_Ctrl_S))	// Save file
			{
				int saved = attoFile_write(&file);
				switch (saved)
				{
				case writeRes_nothingNew:
					attoDS_statusDraw(&editor, L"Nothing new to save");
					break;
				case writeRes_openError:
					attoDS_statusDraw(&editor, L"File open error!");
					break;
				case writeRes_writeError:
					attoDS_statusDraw(&editor, L"File is write-protected!");
					break;
				case writeRes_memError:
					attoDS_statusDraw(&editor, L"Memory allocation error!");
					break;
				default:
				{
					wchar_t tempstr[MAX_STATUS];
					swprintf_s(tempstr, MAX_STATUS, L"Wrote %d bytes.", saved);
					attoDS_statusDraw(&editor, tempstr);
				}
				}
			}
			// Normal keys
			else if (key > sac_last_code)
			{
				wchar_t tempstr[MAX_STATUS];
				swprintf_s(tempstr, MAX_STATUS, L"'%c' #%d", key, keyCount);
				attoDS_statusDraw(&editor, tempstr);
				if (attoFile_addNormalCh(&file, key))
				{
					attoDS_refresh(&editor);
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
						attoDS_statusDraw(&editor, L"\u2191 + 'TAB'");
						wVirtKey = VK_OEM_BACKTAB;
					}
					else
					{
						attoDS_statusDraw(&editor, L"'TAB'");
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
						[VK_UP]     = L"\u2191",
						[VK_DOWN]   = L"\u2193"
					};
					attoDS_statusDraw(&editor, buf[wVirtKey]);
					break;
				}
				case VK_CAPITAL:
				{
					wchar_t tempstr[MAX_STATUS];
					swprintf_s(tempstr, MAX_STATUS, L"'CAPS' %s", (GetKeyState(VK_CAPITAL) & 0x0001) ? L"On" : L"Off");
					attoDS_statusDraw(&editor, tempstr);
					break;
				}
				case VK_NUMLOCK:
				{
					wchar_t tempstr[MAX_STATUS];
					swprintf_s(tempstr, MAX_STATUS, L"'NUMLOCK' %s", (GetKeyState(VK_NUMLOCK) & 0x0001) ? L"On" : L"Off");
					attoDS_statusDraw(&editor, tempstr);
					break;
				}
				case VK_SCROLL:
				{
					wchar_t tempstr[MAX_STATUS];
					swprintf_s(tempstr, MAX_STATUS, L"'SCRLOCK' %s", (GetKeyState(VK_SCROLL) & 0x0001) ? L"On" : L"Off");
					attoDS_statusDraw(&editor, tempstr);
					break;
				}
				}

				if (attoFile_addSpecialCh(&file, wVirtKey))
				{
					attoDS_refresh(&editor);
				}
			}
		}
		else
		{
			boolPut(keybuffer, key, false);
		}
		prevkey = key;
		//prevstate = state;
		memcpy(prevkeybuffer, keybuffer, 32 * sizeof(uint8_t));
		FlushConsoleInputBuffer(editor.conIn);
	}

	return true;
}
void atto_updateScrbuf()
{
	attoFile_updateCury(&file, editor.scrbuf.h - 2);
	file.data.curx = (uint32_t)i32Max(0, (int32_t)file.data.currentNode->curx - (int32_t)editor.scrbuf.w);
	uint32_t size = editor.scrbuf.w * editor.scrbuf.h;
	for (uint32_t i = 0; i < size; ++i)
	{
		editor.scrbuf.mem[i] = L' ';
	}
	attoLineNode * node = file.data.pcury;
	for (uint32_t i = 0; i < editor.scrbuf.h - 1 && node != NULL; ++i)
	{
		// if line is active line
		if (node == file.data.currentNode)
		{
			// Update cursor position
			editor.cursorpos.Y = (int16_t)i;
			editor.cursorpos.X = (int16_t)(node->curx - file.data.curx);
			SetConsoleCursorPosition(editor.scrbuf.handle, editor.cursorpos);
		}
		wchar_t * destination = &editor.scrbuf.mem[i * editor.scrbuf.w];

		// Drawing

		// Advance idx by file.data.curx
		uint32_t idx = 0;

		for (uint32_t i = file.data.curx; i > 0 && idx < node->lineEndx;)
		{
			if (idx == node->curx)
			{
				idx += node->freeSpaceLen;
				continue;
			}
			++idx;
			--i;
		}

		for (uint32_t i = 0; idx < node->lineEndx && i < editor.scrbuf.w;)
		{
			if (idx == node->curx)
			{
				idx += node->freeSpaceLen;
				continue;
			}
			destination[i] = node->line[idx];
			++idx;
			++i;
		}

		node = node->nextNode;
	}
}

uint32_t atto_convToUnicode(const char * utf8, int numBytes, wchar_t ** putf16, uint32_t * sz)
{
	if (numBytes == 0)
	{
		if (sz != NULL && *sz < 1)
		{
			*putf16 = realloc(*putf16, sizeof(wchar_t));
		}
		else if (*putf16 == NULL || sz == NULL)
		{
			*putf16 = malloc(sizeof(wchar_t));
		}

		if (*putf16 != NULL)
		{
			*putf16[0] = L'\0';
		}

		return 0;
	}
	// Query the needed size
	uint32_t size = (uint32_t)MultiByteToWideChar(
		CP_UTF8,
		MB_PRECOMPOSED,
		utf8,
		numBytes,
		NULL,
		0
	);
	// Try to allocate memory
	if (sz != NULL && *sz < size)
	{
		void * mem = realloc(*putf16, size * sizeof(wchar_t));
		if (mem == NULL)
		{
			return 0;
		}
		*putf16 = mem;
		*sz     = size;
	}
	else if (*putf16 == NULL || sz == NULL)
	{
		*putf16 = malloc(size * sizeof(wchar_t));
		if (*putf16 == NULL)
		{
			return 0;
		}
	}

	// Make conversion
	MultiByteToWideChar(
		CP_UTF8,
		MB_PRECOMPOSED,
		utf8,
		numBytes,
		*putf16,
		(int)size
	);
	return size;
}
uint32_t atto_convFromUnicode(const wchar_t * utf16, int numChars, char ** putf8, uint32_t * sz)
{
	if (numChars == 0)
	{
		if (sz != NULL && *sz < 1)
		{
			*putf8 = realloc(*putf8, sizeof(wchar_t));
		}
		else if (*putf8 == NULL || sz == NULL)
		{
			*putf8 = malloc(sizeof(wchar_t));
		}

		if (*putf8 != NULL)
		{
			*putf8[0] = L'\0';
		}

		return 0;
	}
	
	// Quory the needed size
	uint32_t size = (uint32_t)WideCharToMultiByte(
		CP_UTF8,
		0,
		utf16,
		numChars,
		NULL,
		0,
		NULL,
		NULL
	);
	// Alloc mem
	if (sz != NULL && *sz < size)
	{
		void * mem = realloc(*putf8, size * sizeof(char));
		if (mem == NULL)
		{
			return 0;
		}
		*putf8 = mem;
		*sz    = size;
	}
	else if (*putf8 == NULL || sz == NULL)
	{
		*putf8 = malloc(size * sizeof(char));
		if (*putf8 == NULL)
		{
			return 0;
		}
	}

	// Convert
	WideCharToMultiByte(
		CP_UTF8,
		0,
		utf16,
		numChars,
		*putf8,
		(int)size,
		NULL,
		NULL
	);
	return size;
}
uint32_t atto_strnToLines(wchar_t * utf16, uint32_t chars, wchar_t *** lines)
{
	// Count number of newline characters (to count number of lines - 1)
	uint32_t newlines = 1;
	for (uint32_t i = 0; i < chars; ++i)
	{
		if (utf16[i] == '\n')
		{
			++newlines;
		}
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
		else if (utf16[i] == '\r')
		{
			utf16[i] = '\0';
			(*lines)[j] = &utf16[starti];
			starti = i + 2;
			++i;
			++j;
		}
	}
	(*lines)[j] = &utf16[starti];

	return newlines;
}

