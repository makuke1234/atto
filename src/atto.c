#include "atto.h"


i32 min_i32(i32 a, i32 b)
{
	return (a < b) ? a : b;
}
i32 max_i32(i32 a, i32 b)
{
	return (a < b) ? b : a;
}
u32 min_u32(u32 a, u32 b)
{
	return (a < b) ? a : b;
}
u32 max_u32(u32 a, u32 b)
{
	return (a < b) ? b : a;
}

usize min_usize(usize a, usize b)
{
	return (a < b) ? a : b;
}
usize max_usize(usize a, usize b)
{
	return (a < b) ? b : a;
}



static aData_t * s_atExitData = NULL;

void atto_exitHandlerSetVars(aData_t * restrict pdata)
{
	s_atExitData = pdata;
}
void atto_exitHandler(void)
{
	// Clear resources
	aData_destroy(s_atExitData);
}

const wchar * atto_getFileName(int argc, const wchar * const * const restrict argv)
{
	return (argc > 1) ? argv[1] : NULL;
}
void atto_printHelp(const wchar * restrict app)
{
	fwprintf(stderr, L"Correct usage:\n%S [file]\n", app);
}

static const char * atto_errCodes[aerrNUM_OF_ELEMS] = {
	"Uknown error occurred!",
	"Error reading file!",
	"Error initialising window!"
};
void atto_printErr(aErr_e errCode)
{
	errCode = (errCode >= aerrNUM_OF_ELEMS) ? aerrUNKNOWN : errCode;
	fprintf(stderr, "%s\n", atto_errCodes[errCode]);
}

bool atto_loop(aData_t * restrict peditor)
{
	aFile_t * restrict pfile = &peditor->file;
	enum specialASCIIcodes
	{
		sacCTRL_Q = 17,
		sacCTRL_R = 18,
		sacCTRL_S = 19,
		sacCTRL_E = 5,

		sacLAST_CODE = 31
	};

	INPUT_RECORD ir;
	DWORD evRead;
	if (!ReadConsoleInputW(peditor->conIn, &ir, 1, &evRead) || !evRead)
	{
		return true;
	}

	if (ir.EventType == KEY_EVENT)
	{
		static wchar prevkey, prevwVirtKey;

		static u32 keyCount = 1;
		static bool waitingEnc = false;

		wchar key      = ir.Event.KeyEvent.uChar.UnicodeChar;
		wchar wVirtKey = ir.Event.KeyEvent.wVirtualKeyCode;
		const bool keydown = ir.Event.KeyEvent.bKeyDown != 0;

		if (keydown)
		{
			keyCount = ((key == prevkey) && (wVirtKey == prevwVirtKey)) ? (keyCount + 1) : 1;
	
			wchar tempstr[MAX_STATUS];
			bool draw = true;

			if (((wVirtKey == VK_ESCAPE) && (prevwVirtKey != VK_ESCAPE)) || ((key == sacCTRL_Q) && (key != sacCTRL_Q)))	// Exit on Escape or Ctrl+Q
			{
				return false;
			}
			else if (waitingEnc && (key != sacCTRL_E))
			{
				bool done = true;
				switch (wVirtKey)
				{
				// CRLF
				case L'F':
					pfile->eolSeq = eolCRLF;
					break;
				// LF
				case L'L':
					pfile->eolSeq = eolLF;
					break;
				// CR
				case L'C':
					pfile->eolSeq = eolCR;
					break;
				default:
					wcscpy_s(tempstr, MAX_STATUS, L"Unknown EOL combination!");
					done = false;
				}
				if (done)
				{
					swprintf_s(
						tempstr,
						MAX_STATUS,
						L"Using %s%s EOL sequences",
						(pfile->eolSeq & eolCR) ? L"CR" : L"",
						(pfile->eolSeq & eolLF) ? L"LF" : L""
					);
				}

				waitingEnc = false;
			}
			else if ((key == sacCTRL_R) && (prevkey != sacCTRL_R))	// Reload file
			{
				const wchar * res;
				if ((res = aFile_read(pfile)) != NULL)
				{
					wcscpy_s(tempstr, MAX_STATUS, res);
				}
				else
				{
					swprintf_s(
						tempstr,
						MAX_STATUS,
						L"File reloaded successfully! %s%s EOL sequences",
						(pfile->eolSeq & eolCR) ? L"CR" : L"",
						(pfile->eolSeq & eolLF) ? L"LF" : L""
					);
				}
				aData_refresh(peditor);
			}
			else if ((key == sacCTRL_S) && (prevkey != sacCTRL_S))	// Save file
			{
				const isize saved = aFile_write(pfile);
				switch (saved)
				{
				case afwrNOTHING_NEW:
					wcscpy_s(tempstr, MAX_STATUS, L"Nothing new to save");
					break;
				case afwrOPEN_ERROR:
					wcscpy_s(tempstr, MAX_STATUS, L"File open error!");
					break;
				case afwrWRITE_ERROR:
					wcscpy_s(tempstr, MAX_STATUS, L"File is write-protected!");
					break;
				case afwrMEM_ERROR:
					wcscpy_s(tempstr, MAX_STATUS, L"Memory allocation error!");
					break;
				default:
					swprintf_s(tempstr, MAX_STATUS, L"Wrote %zd bytes", saved);
				}
			}
			else if ((key == sacCTRL_E) && (prevkey != sacCTRL_E))
			{
				waitingEnc = true;
				wcscpy_s(tempstr, MAX_STATUS, L"Waiting for EOL combination (F = CRLF, L = LF, C = CR)...");
			}
			// Normal keys
			else if (key > sacLAST_CODE)
			{
				swprintf_s(tempstr, MAX_STATUS, L"'%c' #%u", key, keyCount);
				if (aFile_addNormalCh(pfile, key))
				{
					aData_refresh(peditor);
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
						wcscpy_s(tempstr, MAX_STATUS, L"\u2191 + 'TAB'");
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
					static const wchar * buf[] = {
						[VK_TAB]    = L"'TAB'",
						[VK_RETURN] = L"'RET'",
						[VK_BACK]   = L"'BS'",
						[VK_DELETE] = L"'DEL'",
						[VK_LEFT]   = L"\u2190",
						[VK_RIGHT]  = L"\u2192",
						[VK_UP]     = L"\u2191",
						[VK_DOWN]   = L"\u2193"
					};
					wcscpy_s(tempstr, MAX_STATUS, buf[wVirtKey]);
					break;
				}
				case VK_CAPITAL:
					wcscpy_s(tempstr, MAX_STATUS, (GetKeyState(VK_CAPITAL) & 0x0001) ? L"'CAPS' On" : L"'CAPS' Off");
					break;
				case VK_NUMLOCK:
					wcscpy_s(tempstr, MAX_STATUS, (GetKeyState(VK_NUMLOCK) & 0x0001) ? L"'NUMLOCK' On" : L"'NUMLOCK' Off");
					break;
				case VK_SCROLL:
					wcscpy_s(tempstr, MAX_STATUS, (GetKeyState(VK_SCROLL) & 0x0001) ? L"'SCRLOCK' On" : L"'SCRLOCK' Off");
					break;
				default:
					draw = false;
				}

				if (aFile_addSpecialCh(pfile, wVirtKey))
				{
					aData_refresh(peditor);
				}
			}
			
			if (draw)
			{
				aData_statusDraw(peditor, tempstr);
			}
		}
		else
		{
			key = wVirtKey = 0;
		}

		prevkey = key;
		prevwVirtKey = wVirtKey;
	}

	return true;
}
void atto_updateScrbuf(aData_t * restrict peditor)
{
	aFile_t * restrict pfile = &peditor->file;
	aFile_updateCury(pfile, peditor->scrbuf.h - 2);
	isize delta = (isize)pfile->data.currentNode->curx - (isize)peditor->scrbuf.w - (isize)pfile->data.curx;
	if (delta >= 0)
	{
		pfile->data.curx += (usize)(delta + 1);
	}
	else if (pfile->data.curx > pfile->data.currentNode->curx)
	{
		pfile->data.curx = max_usize(1, pfile->data.currentNode->curx) - 1;
	}
	for (usize i = 0, size = (usize)peditor->scrbuf.w * (usize)peditor->scrbuf.h; i < size; ++i)
	{
		peditor->scrbuf.mem[i] = L' ';
	}
	aLine_t * node = pfile->data.pcury;
	for (u32 i = 0, h1 = peditor->scrbuf.h - 1; i < h1 && node != NULL; ++i)
	{
		// if line is active line
		if (node == pfile->data.currentNode)
		{
			// Update cursor position
			peditor->cursorpos = (COORD){
				.X = (SHORT)min_usize(node->curx - pfile->data.curx, (usize)peditor->scrbuf.w - 1),
				.Y = (SHORT)i
			};
			SetConsoleCursorPosition(peditor->scrbuf.handle, peditor->cursorpos);
		}
		wchar * restrict destination = &peditor->scrbuf.mem[(usize)i * (usize)peditor->scrbuf.w];

		// Drawing

		// Advance idx by file.data.curx
		usize idx = 0;
		for (usize j = pfile->data.curx; j > 0 && idx < node->lineEndx;)
		{
			if (idx == node->curx && node->freeSpaceLen > 0)
			{
				idx += node->freeSpaceLen;
				continue;
			}
			++idx;
			--j;
		}
		for (usize j = 0; idx < node->lineEndx && j < peditor->scrbuf.w;)
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

u32 atto_toutf16(const char * restrict utf8, int numBytes, wchar ** restrict putf16, usize * restrict sz)
{
	// Query the needed size
	const u32 size = (numBytes == 0) ? 1 : (u32)MultiByteToWideChar(
		CP_UTF8,
		MB_PRECOMPOSED,
		utf8,
		numBytes,
		NULL,
		0
	);
	// Try to allocate memory
	if ((sz != NULL) && (*sz < (usize)size))
	{
		vptr mem = realloc(*putf16, (usize)size * sizeof(wchar));
		if (mem == NULL)
		{
			return 0;
		}
		*putf16 = mem;
		*sz     = (usize)size;
	}
	else if ((*putf16 == NULL) || (sz == NULL))
	{
		*putf16 = malloc((usize)size * sizeof(wchar));
		if (*putf16 == NULL)
		{
			return 0;
		}
	}

	if (numBytes != 0)
	{
		// Make conversion
		MultiByteToWideChar(
			CP_UTF8,
			MB_PRECOMPOSED,
			utf8,
			numBytes,
			*putf16,
			(int)size
		);
	}
	else
	{
		(*putf16)[0] = L'\0';
	}
	return size;
}
u32 atto_toutf8(const wchar * restrict utf16, int numChars, char ** restrict putf8, usize * restrict sz)
{
	const u32 size = (numChars == 0) ? 1 : (u32)WideCharToMultiByte(
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
	if ((sz != NULL) && (*sz < size))
	{
		vptr mem = realloc(*putf8, (usize)size * sizeof(char));
		if (mem == NULL)
		{
			return 0;
		}
		*putf8 = mem;
		*sz    = size;
	}
	else if ((*putf8 == NULL) || (sz == NULL))
	{
		*putf8 = malloc((usize)size * sizeof(char));
		if (*putf8 == NULL)
		{
			return 0;
		}
	}

	// Convert
	if (numChars != 0)
	{
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
	}
	else
	{
		(*putf8)[0] = '\0';
	}
	return size;
}
usize atto_strnToLines(wchar * restrict utf16, usize chars, wchar *** restrict lines, eolSeq_e * restrict eolSeq)
{
	// Count number of newline characters (to count number of lines - 1)
	usize newlines = 1;
	
	// Set default EOL sequence
	*eolSeq = eolDEF;
	for (usize i = 0; i < chars; ++i)
	{
		if (utf16[i] == L'\r')
		{
			*eolSeq = ((i + 1 < chars) && (utf16[i+1] == L'\n')) ? eolCRLF : eolCR;
			++newlines;
			i += (*eolSeq == eolCRLF);
		}
		else if (utf16[i] == L'\n')
		{
			*eolSeq = eolLF;
			++newlines;
		}
	}
	*lines = malloc(newlines * sizeof(wchar *));
	if (*lines == NULL)
	{
		return 0;
	}

	bool isCRLF = (*eolSeq == eolCRLF);
	usize starti = 0, j = 0;
	for (usize i = 0; i < chars; ++i)
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
usize atto_tabsToSpaces(wchar ** restrict str, usize * restrict len)
{
	usize realLen = (((len == NULL) || (*len == 0)) ? (wcslen(*str) + 1) : *len), realCap = realLen;

	// Conversion happens here
	wchar * restrict s = *str;

	for (usize i = 0; i < realLen;)
	{
		if (s[i] == L'\t')
		{
			if ((realLen + 3) > realCap)
			{
				realCap = (realLen + 3) * 2;
				s = realloc(s, sizeof(wchar) * realCap);
				if (s == NULL)
				{
					return 0;
				}
			}
			memmove(&s[i + 3], &s[i], sizeof(wchar) * (realLen - i));
			for (u8 j = 0; j < 4; ++i, ++j)
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
	s = realloc(s, sizeof(wchar) * realLen);
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
