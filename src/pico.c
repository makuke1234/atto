#include "pico.h"

#include <stdlib.h>
#include <string.h>

#include "profiling.h"

pico_File file = { 0 };
pico_DS editor = { 0 };

int wmain(int argc, const wchar_t * argv[])
{
	// Initialise profiler, if applicable
	initProfiler();

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
	writeProfiler("wmain", "Starting to read file...");
	if ((res = picoFile_read(&file)) != NULL)
	{
		picoDS_statusDraw(&editor, res);
	}

	picoDS_refresh(&editor);
	while (pico_loop());

	return 0;
}

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

pico_LNode * picoLNode_create(pico_LNode * curnode, pico_LNode * nextnode)
{
	pico_LNode * node = malloc(sizeof(pico_LNode));
	if (node == NULL)
	{
		return NULL;
	}

	if (curnode != NULL)
	{
		// Normal empty line
		if ((curnode->curx + curnode->freeSpaceLen) == curnode->lineEndx)
		{
			node->line = malloc(sizeof(wchar_t) * PICO_LNODE_DEFAULT_FREE);
			if (node->line == NULL)
			{
				free(node);
				return NULL;
			}
			node->lineEndx = PICO_LNODE_DEFAULT_FREE;
		}
		// Copy contents after cursor to this line
		else
		{
			uint32_t contStart = curnode->curx + curnode->freeSpaceLen, contLen = curnode->lineEndx - contStart;
			node->lineEndx = contLen + PICO_LNODE_DEFAULT_FREE;
			node->line = malloc(sizeof(wchar_t) * node->lineEndx);
			if (node->line == NULL)
			{
				free(node);
				return NULL;
			}
			memcpy(node->line + PICO_LNODE_DEFAULT_FREE, &curnode->line[contStart], sizeof(wchar_t) * contLen);
			curnode->freeSpaceLen += contLen;
		}
	}

	node->curx = 0;
	node->freeSpaceLen = PICO_LNODE_DEFAULT_FREE;
	node->prevNode = curnode;
	node->nextNode = nextnode;
	if (curnode != NULL)
	{
		curnode->nextNode  = node;
	}
	if (nextnode != NULL)
	{
		nextnode->prevNode = node;
	}

	return node;
}

pico_LNode * picoLNode_createText(
	pico_LNode * curnode,
	pico_LNode * nextnode,
	const wchar_t * lineText,
	int32_t mText
)
{
	uint32_t maxText;
	if (mText == -1)
	{
		maxText = (uint32_t)wcslen(lineText);
	}
	else
	{
		maxText = (uint32_t)mText;
	}

	pico_LNode * node = malloc(sizeof(pico_LNode));
	if (node == NULL)
	{
		return NULL;
	}

	node->line = malloc(sizeof(wchar_t) * (maxText + PICO_LNODE_DEFAULT_FREE));
	if (node->line == NULL)
	{
		free(node);
		return NULL;
	}
	
	memcpy(node->line, lineText, sizeof(wchar_t) * maxText);

	node->lineEndx = maxText + PICO_LNODE_DEFAULT_FREE;
	node->curx = maxText;
	node->freeSpaceLen = PICO_LNODE_DEFAULT_FREE;

	node->prevNode = curnode;
	node->nextNode = nextnode;
	if (curnode != NULL)
	{
		curnode->nextNode  = node;
	}
	if (nextnode != NULL)
	{
		nextnode->prevNode = node;
	}
	return node;
}

bool picoLNode_realloc(pico_LNode * restrict curnode)
{
	if (curnode->freeSpaceLen == PICO_LNODE_DEFAULT_FREE)
	{
		return true;
	}
	uint32_t totalLen = curnode->lineEndx - curnode->freeSpaceLen;
	void * newmem = realloc(curnode->line, sizeof(wchar_t) * (totalLen + PICO_LNODE_DEFAULT_FREE));
	if (newmem == NULL)
	{
		return false;
	}

	curnode->line = newmem;

	if (curnode->curx != totalLen)
	{
		memmove(
			curnode->line + curnode->curx + PICO_LNODE_DEFAULT_FREE,
			curnode->line + curnode->curx + curnode->freeSpaceLen,
			sizeof(wchar_t) * (totalLen - curnode->curx - curnode->freeSpaceLen)
		);
	}

	curnode->lineEndx = totalLen + PICO_LNODE_DEFAULT_FREE;
	curnode->freeSpaceLen = PICO_LNODE_DEFAULT_FREE;

	return true;
}

bool picoLNode_merge(pico_LNode * restrict node)
{
	if (node->nextNode != NULL)
	{
		return false;
	}
	
	pico_LNode * restrict n = node->nextNode;

	// Allocate more memory for first line
	void * linemem = realloc(node->line, sizeof(wchar_t) * (node->lineEndx - node->freeSpaceLen + n->lineEndx - n->freeSpaceLen + PICO_LNODE_DEFAULT_FREE));
	if (linemem == NULL)
	{
		return false;
	}

	node->line = linemem;

	// Move cursor to end, if needed
	if ((node->curx + node->freeSpaceLen) != node->lineEndx)
	{
		memmove(node->line + node->curx, node->line + node->curx + node->freeSpaceLen, sizeof(wchar_t) * (node->lineEndx - node->curx - node->freeSpaceLen));
		node->curx = node->lineEndx - node->freeSpaceLen;
	}

	// Move also other line's cursor to end if needed
	if ((n->curx + n->freeSpaceLen) != n->lineEndx)
	{
		memmove(n->line + n->curx, n->line + n->curx + n->freeSpaceLen, sizeof(wchar_t) * (n->lineEndx - n->curx - n->freeSpaceLen));
		n->curx = n->lineEndx - n->freeSpaceLen;
	}


	node->freeSpaceLen = PICO_LNODE_DEFAULT_FREE;
	node->lineEndx = node->curx + n->curx + PICO_LNODE_DEFAULT_FREE;

	memcpy(node->line + node->curx + node->freeSpaceLen, n->line, sizeof(wchar_t) * n->curx);
	node->nextNode = n->nextNode;
	node->nextNode->prevNode = node;

	picoLNode_destroy(n); 

	return true;
}

void picoLNode_destroy(pico_LNode * restrict node)
{
	if (node->line != NULL)
	{
		free(node->line);
		node->line = NULL;
	}
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
void picoFile_clearLines(pico_File * restrict file)
{
	pico_LNode * node = file->data.firstNode;
	file->data.firstNode = NULL;
	file->data.currentNode = NULL;
	file->data.cury = 0;
	if (node == NULL)
	{
		return;
	}
	while (node->nextNode != NULL)
	{
		node = node->nextNode;
		free(node->prevNode);
	}
	free(node);
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
	writeProfiler("picoFile_read", "Opened file with size of %u bytes", fileSize);

	// Alloc array
	char * bytes = malloc((fileSize + 1) * sizeof(char));
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
	bytes[fileSize] = '\0';
	++fileSize;
	// Move file pointer back to beginning
	SetFilePointer(file->hFile, 0, NULL, FILE_BEGIN);

	// Convert to UTF-16
	wchar_t * utf16;
	uint32_t chars = pico_convToUnicode(bytes, (int)fileSize, &utf16);
	if (utf16 == NULL)
	{
		free(bytes);
		return L"Unicode conversion error!";
	}
	writeProfiler("picoFile_read", "Converted %u bytes of character to %u UTF-16 characters.", fileSize, chars);
	writeProfiler("picoFile_read", "File UTF-16 contents \"%S\"", utf16);
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
	writeProfiler("picoFile_read", "Total of %u lines", numLines);
	for (int i = 0; i < numLines; ++i)
	{
		writeProfiler("picoFile_read", "Line %d: \"%S\"", i, lines[i]);
	}

	picoFile_clearLines(file);
	if (numLines == 0)
	{
		file->data.firstNode = picoLNode_create(NULL, NULL);
		if (file->data.firstNode == NULL)
		{
			free(lines);
			free(utf16);
			return L"Line creation error!";
		}
	}
	else
	{
		file->data.firstNode = picoLNode_createText(NULL, NULL, lines[0], -1);
		if (file->data.firstNode == NULL)
		{
			free(lines);
			free(utf16);
			return L"Line creation error!";
		}
	}
	file->data.currentNode = file->data.firstNode;
	for (uint32_t i = 1; i < numLines; ++i)
	{
		pico_LNode * node = picoLNode_createText(file->data.currentNode, NULL, lines[i], -1);
		if (node == NULL)
		{
			free(lines);
			free(utf16);
			return L"Line creation error!";
		}
		file->data.currentNode->nextNode = node;
		file->data.currentNode = node;
		++file->data.cury;
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
	pico_LNode * node = file->data.currentNode;

	if (node->freeSpaceLen < 1 && !picoLNode_realloc(node))
	{
		return false;
	}

	node->line[node->curx] = ch;
	++node->curx;
	--node->freeSpaceLen;
	return true;
}
bool picoFile_addSpecialCh(pico_File * restrict file, wchar_t ch)
{
	switch (ch)
	{
	case VK_TAB:
		for (int i = 0; i < 4; ++i)
		{
			if (picoFile_addNormalCh(file, ' ') == false)
			{
				return false;
			}
		}
		break;
	case VK_OEM_BACKTAB:
		// Check if there's 4 spaces before the caret
		if (picoFile_checkLineAt(file, -4, L"    ", 4))
		{
			for (int i = 0; i < 4; ++i)
			{
				picoFile_deleteForward(file);
			}
		}
		// If there isn't, check if there's 4 spaces after the caret
		else if (picoFile_checkLineAt(file, 0, L"    ", 4))
		{
			for (int i = 0; i < 4; ++i)
			{
				picoFile_deleteBackward(file);
			}
		}
		break;
	case VK_RETURN:	// Enter key
		picoFile_addNewLine(file);
		break;
	case VK_BACK:	// Backspace
		picoFile_deleteBackward(file);
		break;
	case VK_DELETE:	// Delete
		picoFile_deleteForward(file);
		break;
	// Move cursor
	case VK_LEFT:	// Left arrow
		if (file->data.currentNode->curx > 0)
		{
			pico_LNode * node = file->data.currentNode;
			node->line[node->curx + node->freeSpaceLen - 1] = node->line[node->curx - 1];
			--node->curx;
		}
		else if (file->data.currentNode->prevNode != NULL)
		{
			file->data.currentNode = file->data.currentNode->prevNode;
		}
		break;
	case VK_RIGHT:	// Right arrow
		if ((file->data.currentNode->curx + file->data.currentNode->freeSpaceLen) < file->data.currentNode->lineEndx)
		{
			pico_LNode * node = file->data.currentNode;
			node->line[node->curx] = node->line[node->curx + node->freeSpaceLen];
			++node->curx;
		}
		else if (file->data.currentNode->nextNode != NULL)
		{
			file->data.currentNode = file->data.currentNode->nextNode;
		}
		break;
	case VK_UP:		// Up arrow
		if (file->data.currentNode->prevNode != NULL)
		{
			file->data.currentNode = file->data.currentNode->prevNode;
		}
		break;
	case VK_DOWN:	// Down arrow
		if (file->data.currentNode->nextNode != NULL)
		{
			file->data.currentNode = file->data.currentNode->nextNode;
		}
		break;
	default:
		return false;
	}

	return true;
}

bool picoFile_checkLineAt(const pico_File * restrict file, int32_t maxdelta, const wchar_t * string, uint32_t maxString)
{
	const pico_LNode * restrict cl = file->data.currentNode;
	if (maxdelta >= 0 && cl->curx == cl->lineEndx)
	{
		return false;
	}
	if (maxdelta >= 0)
	{
		uint32_t startx = cl->curx + cl->freeSpaceLen - 1;
		startx += u32Min((uint32_t)maxdelta, cl->lineEndx - startx + 1);
		return wcsncmp(cl->line + startx, string, maxString) == 0;
	}
	else
	{
		uint32_t startx = cl->curx;
		maxdelta = i32Min(maxdelta, (int32_t)startx);
		uint32_t maxcomp = u32Min((uint32_t)maxdelta, maxString);
		bool ret = wcsncmp(cl->line + startx - maxdelta, string, maxcomp) == 0;
		if (ret == false)
		{
			return false;
		}
		if (maxString > maxcomp)
		{
			if ((cl->curx + cl->freeSpaceLen) < cl->lineEndx)
			{
				string += maxcomp;
				maxString -= maxcomp;
				return wcsncmp(cl->line + cl->curx + cl->freeSpaceLen, string, maxString) == 0;
			}
			else
			{
				return false;
			}
		}
		return true;
	}
}
bool picoFile_deleteForward(pico_File * restrict file)
{
	pico_LNode * node = file->data.currentNode;
	if ((node->curx + node->freeSpaceLen) < node->lineEndx)
	{
		++node->freeSpaceLen;
		return true;
	}
	else if (node->nextNode != NULL)
	{
		return picoLNode_merge(node);
	}
	else
	{
		return false;
	}
}
bool picoFile_deleteBackward(pico_File * restrict file)
{
	pico_LNode * node = file->data.currentNode;
	if (node->curx > 0)
	{
		--node->curx;
		++node->freeSpaceLen;
		return true;
	}
	else if (node->prevNode != NULL)
	{
		// Add current node data to previous node data
		file->data.currentNode = node->prevNode;
		return picoLNode_merge(file->data.currentNode);
	}
	else
	{
		return false;
	}
}
bool picoFile_addNewLine(pico_File * restrict file)
{
	pico_LNode * node = picoLNode_create(file->data.currentNode, file->data.currentNode->nextNode);
	if (node == NULL)
	{
		return false;
	}

	file->data.currentNode->nextNode = node;
	file->data.currentNode = node;
	return true;
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
	{
		return false;
	}

	ds->scrbuf.w = (uint32_t)(csbi.srWindow.Right  - csbi.srWindow.Left + 1);
	ds->scrbuf.h = (uint32_t)(csbi.srWindow.Bottom - csbi.srWindow.Top  + 1);
	// Create screen buffer
	ds->scrbuf.handle = CreateConsoleScreenBuffer(
		GENERIC_WRITE,
		0,
		NULL,
		CONSOLE_TEXTMODE_BUFFER,
		NULL
	);
	if (ds->scrbuf.handle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	ds->scrbuf.mem = malloc((size_t)(ds->scrbuf.w * ds->scrbuf.h) * sizeof(wchar_t));
	if (ds->scrbuf.mem == NULL)
	{
		return false;
	}

	for (uint32_t i = 0, sz = ds->scrbuf.w * ds->scrbuf.h; i < sz; ++i)
	{
		ds->scrbuf.mem[i] = L' ';
	}
	if (!SetConsoleScreenBufferSize(ds->scrbuf.handle, (COORD){ .X = (SHORT)ds->scrbuf.w, .Y = (SHORT)ds->scrbuf.h }))
	{
		return false;
	}
	if (!SetConsoleActiveScreenBuffer(ds->scrbuf.handle))
	{
		return false;
	}

	return true;
}
void picoDS_refresh(pico_DS * restrict ds)
{
	pico_updateScrbuf();
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		ds->scrbuf.handle,
		ds->scrbuf.mem,
		ds->scrbuf.w * (ds->scrbuf.h - 1),
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
		ds->scrbuf.w * ds->scrbuf.h,
		(COORD){ 0, 0 },
		&dwBytes
	);
}
void picoDS_statusDraw(pico_DS * restrict ds, const wchar_t * message)
{
	size_t len = wcslen(message), effLen = (len > ds->scrbuf.w) ? (size_t)ds->scrbuf.w : len;
	wchar_t * restrict lastLine = ds->scrbuf.mem + (ds->scrbuf.h - 1) * ds->scrbuf.w;
	memcpy(
		lastLine,
		message,
		sizeof(wchar_t) * effLen
	);
	for (size_t i = effLen; i < ds->scrbuf.w; ++i)
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
		ds->scrbuf.mem + (ds->scrbuf.h - 1) * ds->scrbuf.w,
		ds->scrbuf.w,
		(COORD){ .X = 0, .Y = (SHORT)(ds->scrbuf.h - 1) },
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
	{
		errCode = picoE_unknown;
	}

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
			else if (boolGet(keybuffer, sac_Ctrl_S) && !boolGet(prevkeybuffer, sac_Ctrl_S))	// Save file
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
			boolPut(keybuffer, key, false);
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
	if (file.data.cury == NULL)
	{
		file.data.cury = file.data.currentNode;
	}
	file.data.curx = i32Max(0, (int32_t)file.data.cury->curx - (int32_t)editor.scrbuf.w);
	uint32_t size = editor.scrbuf.w * editor.scrbuf.h;
	for (uint32_t i = 0; i < size; ++i)
	{
		editor.scrbuf.mem[i] = L' ';
	}
	pico_LNode * node = file.data.cury;
	for (uint32_t i = 0; i < editor.scrbuf.h; ++i)
	{
		if (node == NULL)
		{
			break;
		}

		// if line is active line
		if (node == file.data.currentNode)
		{
			// Update cursor position
			editor.cursorpos.Y = i;
			editor.cursorpos.X = node->curx - file.data.curx;
			SetConsoleCursorPosition(editor.scrbuf.handle, editor.cursorpos);
		}
		wchar_t * destination = &editor.scrbuf.mem[i * editor.scrbuf.w];

		if (file.data.curx <= node->curx)
		{
			memcpy(destination, node->line + file.data.curx, sizeof(wchar_t) * (node->curx - file.data.curx));
			memcpy(destination + node->curx - file.data.curx, node->line + node->curx + node->freeSpaceLen, sizeof(wchar_t) * (node->lineEndx - node->curx - node->freeSpaceLen));
		}
		if ((node->curx + node->freeSpaceLen) != node->lineEndx)
		{
			destination += node->curx;
			uint32_t maxSize = editor.scrbuf.w - node->curx;
		}

		node = node->nextNode;
	}
}

uint32_t pico_convToUnicode(const char * utf8, int numBytes, wchar_t ** putf16)
{
	if (numBytes == 0)
	{
		*putf16 = malloc(sizeof(wchar_t));
		if (*putf16 != NULL)
		{
			*putf16[0] = L'\0';
		}

		return 0;
	}
	// Query the needed size
	int size = MultiByteToWideChar(
		CP_UTF8,
		MB_PRECOMPOSED,
		utf8,
		numBytes,
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
		numBytes,
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
