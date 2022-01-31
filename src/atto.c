#include "atto.h"
#include "profiling.h"


bool boolGet(uint8_t * restrict arr, size_t index)
{
	return (arr[index / 8] & (0x01 << (index % 8))) != 0;
}
void boolPut(uint8_t * restrict arr, size_t index, bool value)
{
	const size_t in1 = index / 8;
	const uint8_t pattern = 0x01 << (index % 8);
	value ? (arr[in1] |= pattern) : (arr[in1] &= (uint8_t)~pattern);
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


static attoData_t * s_atExitData = NULL;

void atto_exitHandlerSetVars(attoData_t * pdata)
{
	s_atExitData = pdata;
}
void atto_exitHandler(void)
{
	// Clear resources
	attoData_destroy(s_atExitData);
}

const wchar_t * atto_getFileName(int argc, const wchar_t * const * const argv)
{
	return (argc > 1) ? argv[1] : NULL;
}
void atto_printHelp(const wchar_t * restrict app)
{
	fwprintf(stderr, L"Correct usage:\n%S [file]\n", app);
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
	fprintf(stderr, "%s\n", atto_errCodes[errCode]);
}

bool atto_loop(attoData_t * restrict peditor)
{
	attoFile_t * restrict pfile = &peditor->file;
	enum SpecialAsciiCodes
	{
		sac_Ctrl_Q = 17,
		sac_Ctrl_R = 18,
		sac_Ctrl_S = 19,
		sac_Ctrl_E = 5,

		sac_last_code = 31
	};

	INPUT_RECORD ir;
	DWORD evRead;
	if (!ReadConsoleInputW(peditor->conIn, &ir, 1, &evRead))
	{
		return true;
	}
	if (evRead && ir.EventType == KEY_EVENT)
	{
		static uint8_t keybuffer[32] = { 0 }, prevkeybuffer[32] = { 0 };
		static wchar_t prevkey;

		static int keyCount = 1;
		static bool waitingEnc = false;

		wchar_t key      = ir.Event.KeyEvent.uChar.UnicodeChar;
		wchar_t wVirtKey = ir.Event.KeyEvent.wVirtualKeyCode;
		bool keydown     = ir.Event.KeyEvent.bKeyDown != 0;

		if (keydown)
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

		if (keydown)
		{
			boolPut(keybuffer, key, true);
			if (wVirtKey == VK_ESCAPE || key == sac_Ctrl_Q)	// Exit on Escape or Ctrl+Q
			{
				return false;
			}
			else if (waitingEnc)
			{
				bool done = true;
				switch (wVirtKey)
				{
				// CRLF
				case L'F':
					pfile->eolSeq = EOL_CRLF;
					break;
				// LF
				case L'L':
					pfile->eolSeq = EOL_LF;
					break;
				// CR
				case L'C':
					pfile->eolSeq = EOL_CR;
					break;
				default:
					attoData_statusDraw(peditor, L"Unknown EOL combination!");
					done = false;
				}
				if (done)
				{
					wchar_t tempstr[MAX_STATUS];
					swprintf_s(
						tempstr,
						MAX_STATUS,
						L"Using %s%s line endings.",
						(pfile->eolSeq & EOL_CR) ? L"CR" : L"",
						(pfile->eolSeq & EOL_LF) ? L"LF" : L""
					);
					attoData_statusDraw(peditor, tempstr);
				}

				waitingEnc = false;
			}
			else if (boolGet(keybuffer, sac_Ctrl_R) && !boolGet(prevkeybuffer, sac_Ctrl_R))	// Reload file
			{
				const wchar_t * res;
				if ((res = attoFile_read(pfile)) != NULL)
				{
					attoData_statusDraw(peditor, res);
				}
				else
				{
					wchar_t tempstr[MAX_STATUS];
					swprintf_s(
						tempstr,
						MAX_STATUS,
						L"File reloaded successfully! %s%s line endings.",
						(pfile->eolSeq & EOL_CR) ? L"CR" : L"",
						(pfile->eolSeq & EOL_LF) ? L"LF" : L""
					);
					attoData_statusDraw(peditor, tempstr);
				}
				attoData_refresh(peditor);
			}
			else if (boolGet(keybuffer, sac_Ctrl_S) && !boolGet(prevkeybuffer, sac_Ctrl_S))	// Save file
			{
				int saved = attoFile_write(pfile);
				switch (saved)
				{
				case writeRes_nothingNew:
					attoData_statusDraw(peditor, L"Nothing new to save");
					break;
				case writeRes_openError:
					attoData_statusDraw(peditor, L"File open error!");
					break;
				case writeRes_writeError:
					attoData_statusDraw(peditor, L"File is write-protected!");
					break;
				case writeRes_memError:
					attoData_statusDraw(peditor, L"Memory allocation error!");
					break;
				default:
				{
					wchar_t tempstr[MAX_STATUS];
					swprintf_s(tempstr, MAX_STATUS, L"Wrote %d bytes.", saved);
					attoData_statusDraw(peditor, tempstr);
				}
				}
			}
			else if (boolGet(keybuffer, sac_Ctrl_E) && !boolGet(prevkeybuffer, sac_Ctrl_E))
			{
				waitingEnc = true;
				attoData_statusDraw(peditor, L"Waiting for EOL combination (F = CRLF, L = LF, C = CR)...");
			}
			// Normal keys
			else if (key > sac_last_code)
			{
				wchar_t tempstr[MAX_STATUS];
				swprintf_s(tempstr, MAX_STATUS, L"'%c' #%d", key, keyCount);
				attoData_statusDraw(peditor, tempstr);
				if (attoFile_addNormalCh(pfile, key))
				{
					attoData_refresh(peditor);
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
						attoData_statusDraw(peditor, L"\u2191 + 'TAB'");
						wVirtKey = VK_OEM_BACKTAB;
						break;
					}
					/* fall through */
				case VK_RETURN:	// Enter key
				case VK_BACK:	// Backspace
				case VK_DELETE:	// Delete
				case VK_LEFT:	// Left arrow
				case VK_RIGHT:	// Right arrow
				case VK_UP:		// Up arrow
				case VK_DOWN:	// Down arrow
				{
					static const wchar_t * buf[] = {
						[VK_TAB]    = L"'TAB'",
						[VK_RETURN] = L"'RET'",
						[VK_BACK]   = L"'BS'",
						[VK_DELETE] = L"'DEL'",
						[VK_LEFT]   = L"\u2190",
						[VK_RIGHT]  = L"\u2192",
						[VK_UP]     = L"\u2191",
						[VK_DOWN]   = L"\u2193"
					};
					attoData_statusDraw(peditor, buf[wVirtKey]);
					break;
				}
				case VK_CAPITAL:
				{
					wchar_t tempstr[MAX_STATUS];
					swprintf_s(tempstr, MAX_STATUS, L"'CAPS' %s", (GetKeyState(VK_CAPITAL) & 0x0001) ? L"On" : L"Off");
					attoData_statusDraw(peditor, tempstr);
					break;
				}
				case VK_NUMLOCK:
				{
					wchar_t tempstr[MAX_STATUS];
					swprintf_s(tempstr, MAX_STATUS, L"'NUMLOCK' %s", (GetKeyState(VK_NUMLOCK) & 0x0001) ? L"On" : L"Off");
					attoData_statusDraw(peditor, tempstr);
					break;
				}
				case VK_SCROLL:
				{
					wchar_t tempstr[MAX_STATUS];
					swprintf_s(tempstr, MAX_STATUS, L"'SCRLOCK' %s", (GetKeyState(VK_SCROLL) & 0x0001) ? L"On" : L"Off");
					attoData_statusDraw(peditor, tempstr);
					break;
				}
				}

				if (attoFile_addSpecialCh(pfile, wVirtKey))
				{
					attoData_refresh(peditor);
				}
			}
		}
		else
		{
			boolPut(keybuffer, key, false);
		}
		prevkey = key;
		memcpy(prevkeybuffer, keybuffer, 32 * sizeof(uint8_t));
		FlushConsoleInputBuffer(peditor->conIn);
	}

	return true;
}
void atto_updateScrbuf(attoData_t * restrict peditor)
{
	attoFile_t * restrict pfile = &peditor->file;
	attoFile_updateCury(pfile, peditor->scrbuf.h - 2);
	int32_t delta = (int32_t)pfile->data.currentNode->curx - (int32_t)peditor->scrbuf.w - (int32_t)pfile->data.curx;
	if (delta >= 0)
	{
		pfile->data.curx += (uint32_t)(delta + 1);
	}
	else if (pfile->data.curx > pfile->data.currentNode->curx)
	{
		pfile->data.curx = u32Max(1, pfile->data.currentNode->curx) - 1;
	}
	uint32_t size = peditor->scrbuf.w * peditor->scrbuf.h;
	for (uint32_t i = 0; i < size; ++i)
	{
		peditor->scrbuf.mem[i] = L' ';
	}
	attoLineNode_t * node = pfile->data.pcury;
	for (uint32_t i = 0; i < peditor->scrbuf.h - 1 && node != NULL; ++i)
	{
		// if line is active line
		if (node == pfile->data.currentNode)
		{
			// Update cursor position
			peditor->cursorpos = (COORD){ .X = (int16_t)u32Min(node->curx - pfile->data.curx, peditor->scrbuf.w - 1), .Y = (int16_t)i };
			SetConsoleCursorPosition(peditor->scrbuf.handle, peditor->cursorpos);
		}
		wchar_t * destination = &peditor->scrbuf.mem[i * peditor->scrbuf.w];

		// Drawing

		// Advance idx by file.data.curx
		uint32_t idx = 0;
		for (uint32_t j = pfile->data.curx; j > 0 && idx < node->lineEndx;)
		{
			if (idx == node->curx && node->freeSpaceLen > 0)
			{
				idx += node->freeSpaceLen;
				continue;
			}
			++idx;
			--j;
		}
		for (uint32_t j = 0; idx < node->lineEndx && j < peditor->scrbuf.w;)
		{
			if (idx == node->curx && node->freeSpaceLen > 0)
			{
				idx += node->freeSpaceLen;
				continue;
			}
			destination[j] = node->line[idx];
			++idx;
			++j;
		}

		node = node->nextNode;
	}
}

uint32_t atto_convToUnicode(const char * restrict utf8, int numBytes, wchar_t ** restrict putf16, uint32_t * restrict sz)
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
uint32_t atto_convFromUnicode(const wchar_t * restrict utf16, int numChars, char ** restrict putf8, uint32_t * restrict sz)
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
uint32_t atto_strnToLines(wchar_t * restrict utf16, uint32_t chars, wchar_t *** restrict lines, enum attoEOLsequence * restrict eolSeq)
{
	assert(utf16 != NULL);
	assert(lines != NULL);
	assert(eolSeq != NULL);

	// Count number of newline characters (to count number of lines - 1)
	uint32_t newlines = 1;
	
	// Set default EOL sequence
	*eolSeq = EOL_def;
	for (uint32_t i = 0; i < chars; ++i)
	{
		if (utf16[i] == L'\r')
		{
			*eolSeq = ((i + 1 < chars) && (utf16[i+1] == L'\n')) ? EOL_CRLF : EOL_CR;
			++newlines;
			i += (*eolSeq == EOL_CRLF);
		}
		else if (utf16[i] == L'\n')
		{
			*eolSeq = EOL_LF;
			++newlines;
		}
	}
	*lines = malloc(newlines * sizeof(wchar_t *));
	if (*lines == NULL)
	{
		return 0;
	}

	bool isCRLF = (*eolSeq == EOL_CRLF);
	uint32_t starti = 0, j = 0;
	for (uint32_t i = 0; i < chars; ++i)
	{
		if (utf16[i] == L'\n' || utf16[i] == L'\r')
		{
			utf16[i] = L'\0';
			(*lines)[j] = &utf16[starti];
			starti = i + 1 + isCRLF;
			i += isCRLF;
			++j;
		}
	}
	(*lines)[j] = &utf16[starti];

	return newlines;
}
uint32_t atto_tabsToSpaces(wchar_t ** restrict str, uint32_t * restrict len)
{
	uint32_t realLen = ((len == NULL || *len == 0) ? (uint32_t)wcslen(*str) + 1 : *len), realCap = realLen;

	// Conversion happens here
	wchar_t * s = *str;

	for (uint32_t i = 0; i < realLen;)
	{
		if (s[i] == L'\t')
		{
			if ((realLen + 3) > realCap)
			{
				realCap = (realLen + 3) * 2;
				s = realloc(s, sizeof(wchar_t) * realCap);
				if (s == NULL)
				{
					return 0;
				}
			}
			memmove(&s[i + 3], &s[i], sizeof(wchar_t) * (realLen - i));
			for (uint32_t j = 0; j < 4; ++i, ++j)
			{
				s[i] = L' ';
			}
			realLen += 3;
		}
		else
		{
			++i;
		}
	}

	*str = s;
	// Shrink string
	s = realloc(s, sizeof(wchar_t) * realLen);
	if (s != NULL)
	{
		*str = s;
	}
	
	if (len != NULL)
	{
		*len = realLen;
	}
	return realLen;
}
