#include "atto.h"

#include <stdlib.h>
#include <string.h>

#include "profiling.h"

atto_File file = { .hFile = INVALID_HANDLE_VALUE };
atto_DS editor = { 0 };

int wmain(int argc, const wchar_t * argv[])
{
	// Initialise profiler, if applicable
	initProfiler();

	const wchar_t * fileName = atto_getFileName(argc, argv);
	if (fileName == NULL)
	{
		atto_printHelp(argv[0]);
		return 1;
	}

	if (attoFile_open(&file, fileName, false) == false)
	{
		atto_printErr(attoE_file);
		return 2;
	}
	attoFile_close(&file);

	// Set console title
	attoFile_setConTitle(&file);

	if (!attoDS_init(&editor))
	{
		atto_printErr(attoE_window);
		return 3;
	}

	const wchar_t * res;
	writeProfiler("wmain", "Starting to read file...");
	if ((res = attoFile_read(&file)) != NULL)
	{
		attoDS_statusDraw(&editor, res);
	}

	attoDS_refresh(&editor);
	while (atto_loop());

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

atto_LNode * attoLNode_create(atto_LNode * curnode, atto_LNode * nextnode)
{
	atto_LNode * node = malloc(sizeof(atto_LNode));
	if (node == NULL)
	{
		return NULL;
	}

	if (curnode != NULL)
	{
		// Normal empty line
		if ((curnode->curx + curnode->freeSpaceLen) == curnode->lineEndx)
		{
			node->line = malloc(sizeof(wchar_t) * atto_LNODE_DEFAULT_FREE);
			if (node->line == NULL)
			{
				free(node);
				return NULL;
			}
			node->lineEndx = atto_LNODE_DEFAULT_FREE;
		}
		// Copy contents after cursor to this line
		else
		{
			uint32_t contStart = curnode->curx + curnode->freeSpaceLen, contLen = curnode->lineEndx - contStart;
			node->lineEndx = contLen + atto_LNODE_DEFAULT_FREE;
			node->line = malloc(sizeof(wchar_t) * node->lineEndx);
			if (node->line == NULL)
			{
				free(node);
				return NULL;
			}
			memcpy(node->line + atto_LNODE_DEFAULT_FREE, curnode->line + contStart, sizeof(wchar_t) * contLen);
			curnode->freeSpaceLen += contLen;
		}
	}

	node->curx = 0;
	node->freeSpaceLen = atto_LNODE_DEFAULT_FREE;
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

atto_LNode * attoLNode_createText(
	atto_LNode * curnode,
	atto_LNode * nextnode,
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

	atto_LNode * node = malloc(sizeof(atto_LNode));
	if (node == NULL)
	{
		return NULL;
	}

	node->lineEndx = maxText + atto_LNODE_DEFAULT_FREE;
	node->line = malloc(sizeof(wchar_t) * node->lineEndx);
	if (node->line == NULL)
	{
		free(node);
		return NULL;
	}
	
	memcpy(node->line, lineText, sizeof(wchar_t) * maxText);

	node->curx = maxText;
	node->freeSpaceLen = atto_LNODE_DEFAULT_FREE;

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

bool attoLNode_getText(const atto_LNode * node, wchar_t ** text, uint32_t * tarrsz)
{
	if (text == NULL)
	{
		return false;
	}
	uint32_t totalLen = node->lineEndx - node->freeSpaceLen + 1;

	if (tarrsz != NULL && *tarrsz < totalLen)
	{
		wchar_t * mem = realloc(*text, sizeof(wchar_t) * totalLen);
		if (mem == NULL)
		{
			return false;
		}
		*text   = mem;
		*tarrsz = totalLen;
	}
	else if (tarrsz == NULL)
	{
		*text = malloc(sizeof(wchar_t) * totalLen);
		if (*text == NULL)
		{
			return false;
		}
	}

	wchar_t * t = *text;
	for (uint32_t i = 0; i < node->lineEndx;)
	{
		if (i == node->curx)
		{
			i += node->freeSpaceLen;
			continue;
		}

		*t = node->line[i];
		++t;

		++i;
	}

	*t = L'\0';

	return true;
}

bool attoLNode_realloc(atto_LNode * restrict curnode)
{
	if (curnode->freeSpaceLen == atto_LNODE_DEFAULT_FREE)
	{
		return true;
	}
	uint32_t totalLen = curnode->lineEndx - curnode->freeSpaceLen;
	void * newmem = realloc(curnode->line, sizeof(wchar_t) * (totalLen + atto_LNODE_DEFAULT_FREE));
	if (newmem == NULL)
	{
		return false;
	}

	curnode->line = newmem;

	if (curnode->curx != totalLen)
	{
		memmove(
			curnode->line + curnode->curx + atto_LNODE_DEFAULT_FREE,
			curnode->line + curnode->curx + curnode->freeSpaceLen,
			sizeof(wchar_t) * (totalLen - curnode->curx - curnode->freeSpaceLen)
		);
	}

	curnode->lineEndx     = totalLen + atto_LNODE_DEFAULT_FREE;
	curnode->freeSpaceLen = atto_LNODE_DEFAULT_FREE;

	return true;
}

bool attoLNode_merge(atto_LNode * restrict node, atto_LNode ** restrict ppcury)
{
	if (node->nextNode == NULL)
	{
		return false;
	}
	
	atto_LNode * restrict n = node->nextNode;

	if (*ppcury == n)
	{
		*ppcury = node;
	}

	// Allocate more memory for first line
	void * linemem = realloc(node->line, sizeof(wchar_t) * (node->lineEndx - node->freeSpaceLen + n->lineEndx - n->freeSpaceLen + atto_LNODE_DEFAULT_FREE));
	if (linemem == NULL)
	{
		return false;
	}
	node->line = linemem;

	// Move cursor to end, if needed
	attoLNode_moveCursor(node, (int32_t)node->lineEndx);
	attoLNode_moveCursor(n, (int32_t)n->lineEndx);

	node->freeSpaceLen = atto_LNODE_DEFAULT_FREE;
	node->lineEndx = node->curx + n->curx + atto_LNODE_DEFAULT_FREE;

	memcpy(node->line + node->curx + node->freeSpaceLen, n->line, sizeof(wchar_t) * n->curx);
	node->nextNode = n->nextNode;
	if (node->nextNode != NULL)
	{
		node->nextNode->prevNode = node;
	}

	attoLNode_destroy(n); 

	return true;
}

void attoLNode_moveCursor(atto_LNode * restrict node, int32_t delta)
{
	if (delta < 0)
	{
		for (; delta < 0 && node->curx > 0; ++delta)
		{
			node->line[node->curx + node->freeSpaceLen - 1] = node->line[node->curx - 1];
			--node->curx;
		}
	}
	else
	{
		for (uint32_t total = node->lineEndx - node->freeSpaceLen; delta > 0 && node->curx < total; --delta)
		{
			node->line[node->curx] = node->line[node->curx + node->freeSpaceLen];
			++node->curx;
		}
	}
}

void attoLNode_destroy(atto_LNode * restrict node)
{
	if (node->line != NULL)
	{
		free(node->line);
		node->line = NULL;
	}
	free(node);
}

bool attoFile_open(atto_File * restrict file, const wchar_t * restrict fileName, bool writemode)
{
	if (fileName == NULL)
	{
		fileName = file->fileName;
	}

	// try to open file
	file->hFile = CreateFileW(
		fileName,
		writemode ? GENERIC_WRITE : GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		writemode ? CREATE_ALWAYS : OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (file->hFile == INVALID_HANDLE_VALUE)
	{
		file->canWrite = false;
		return false;
	}
	else
	{
		file->canWrite = writemode;
		file->fileName = fileName;
		return true;
	}
}
void attoFile_close(atto_File * restrict file)
{
	if (file->hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file->hFile);
		file->hFile = INVALID_HANDLE_VALUE;
	}
}
void attoFile_clearLines(atto_File * restrict file)
{
	atto_LNode * node = file->data.firstNode;
	file->data.firstNode = NULL;
	file->data.currentNode = NULL;
	file->data.pcury = NULL;
	if (node == NULL)
	{
		return;
	}
	while (node != NULL)
	{
		atto_LNode * next = node->nextNode;
		attoLNode_destroy(node);
		node = next;
	}
}
const wchar_t * attoFile_read(atto_File * restrict file)
{
	if (attoFile_open(file, NULL, false) == false)
	{
		return L"File opening error!\n";
	}

	// Clear lines

	attoFile_clearLines(file);

	// Read file contents

	// Get file size
	DWORD fileSize = GetFileSize(file->hFile, NULL);
	writeProfiler("attoFile_read", "Opened file with size of %u bytes", fileSize);

	// Alloc array
	char * bytes = malloc((fileSize + 1) * sizeof(char));
	if (bytes == NULL)
	{
		return L"Memory error!";
	}
	BOOL readFileRes = ReadFile(
		file->hFile,
		bytes,
		fileSize,
		NULL,
		NULL
	);
	attoFile_close(file);

	if (!readFileRes)
	{
		free(bytes);
		return L"File read error!";
	}
	bytes[fileSize] = '\0';
	++fileSize;
	// Move file pointer back to beginning

	// Convert to UTF-16
	wchar_t * utf16;
	uint32_t chars = atto_convToUnicode(bytes, (int)fileSize, &utf16, NULL);
	free(bytes);

	if (utf16 == NULL)
	{
		return L"Unicode conversion error!";
	}
	writeProfiler("attoFile_read", "Converted %u bytes of character to %u UTF-16 characters.", fileSize, chars);
	writeProfiler("attoFile_read", "File UTF-16 contents \"%S\"", utf16);
	// Free loaded file memory

	// Save lines to structure
	wchar_t ** lines;
	uint32_t numLines = atto_strnToLines(utf16, chars, &lines);
	if (lines == NULL)
	{
		free(utf16);
		return L"Line reading error!";
	}
	writeProfiler("attoFile_read", "Total of %u lines", numLines);
	for (uint32_t i = 0; i < numLines; ++i)
	{
		writeProfiler("attoFile_read", "Line %d: \"%S\"", i, lines[i]);
	}

	attoFile_clearLines(file);
	if (numLines == 0)
	{
		file->data.firstNode = attoLNode_create(NULL, NULL);
		if (file->data.firstNode == NULL)
		{
			free(lines);
			free(utf16);
			return L"Line creation error!";
		}
	}
	else
	{
		file->data.firstNode = attoLNode_createText(NULL, NULL, lines[0], -1);
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
		atto_LNode * node = attoLNode_createText(file->data.currentNode, NULL, lines[i], -1);
		if (node == NULL)
		{
			free(lines);
			free(utf16);
			return L"Line creation error!";
		}
		file->data.currentNode = node;
	}

	// Free lines arr
	free(lines);
	// Free UTF-16 converted string
	free(utf16);

	return NULL;
}
int attoFile_write(atto_File * restrict file)
{
	if (attoFile_open(file, NULL, true) == false)
	{
		return writeRes_openError;
	}

	if (file->canWrite == false)
	{
		attoFile_close(file);
		return writeRes_writeError;
	}

	writeProfiler("attoFile_write", "Opened file for writing");

	// Save to file here

	wchar_t * lines = NULL, * line = NULL;
	uint32_t linesCap = 0, linesLen = 0, lineCap = 0;

	atto_LNode * node = file->data.firstNode;

	while (node != NULL)
	{
		if (attoLNode_getText(node, &line, &lineCap) == false)
		{
			writeProfiler("attoFile_write", "Failed to fetch line!");
			if (line != NULL)
			{
				free(line);
			}
			if (lines != NULL)
			{
				free(lines);
			}
			return writeRes_memError;
		}

		uint32_t lineLen = (uint32_t)wcsnlen(line, lineCap);

		writeProfiler("attoFile_write", "Got line with size of %u characters. Line contents: \"%S\"", lineLen, line);

		bool addnewline = node->nextNode != NULL;

		uint32_t newLinesLen = linesLen + lineLen + addnewline;

		// Add line to lines, concatenate \n character, if necessary

		if (newLinesLen > linesCap)
		{
			// Resize lines array
			uint32_t newCap = (newLinesLen + 1) * 2;

			void * mem = realloc(lines, sizeof(wchar_t) * newCap);
			if (mem == NULL)
			{
				if (line != NULL)
				{
					free(line);
				}
				if (lines != NULL)
				{
					free(lines);
				}
				return writeRes_memError;
			}

			lines    = mem;
			linesCap = newCap;

			writeProfiler("attoFile_write", "Resized line string. New cap, length is %u, %u bytes.", linesCap, linesLen);
		}

		// Copy line
		memcpy(lines + linesLen, line, sizeof(wchar_t) * lineLen);
		linesLen = newLinesLen;

		if (addnewline)
		{
			lines[linesLen - 1] = L'\n';
		}
		lines[linesLen] = L'\0';

		node = node->nextNode;
	}

	free(line);

	writeProfiler("attoFile_write", "All file contents (%u): \"%S\"", linesLen, lines);

	// Try to convert lines string to UTF-8

	char * utf8 = NULL;
	uint32_t utf8sz = 0;
	atto_convFromUnicode(lines, (int)linesLen, &utf8, &utf8sz);

	// Free UTF-16 lines string
	free(lines);

	// Error-check for conversion
	
	if (utf8 == NULL)
	{
		attoFile_close(file);
		return writeRes_memError;
	}

	writeProfiler("attoFile_write", "Converted UTF-16 string to UTF-8 string");
	writeProfiler("attoFile_write", "UTF-8 contents: \"%s\"", utf8);

	// Try to write UTF-8 lines string to file
	DWORD dwWritten;
	BOOL res = WriteFile(
		file->hFile,
		utf8,
		utf8sz,
		&dwWritten,
		NULL
	);

	// Close file
	attoFile_close(file);

	// Free utf8 string
	free(utf8);

	// Do error checking
	if (!res)
	{
		writeProfiler("attoFile_write", "Error writing to file");
		return writeRes_writeError;
	}
	else
	{
		return (int)dwWritten;
	}
}
void attoFile_setConTitle(atto_File * restrict file)
{
	wchar_t wndName[MAX_PATH];
	size_t fnamelen = wcslen(file->fileName);
	memcpy(wndName, file->fileName, fnamelen * sizeof(wchar_t));
	wcscpy_s(wndName + fnamelen, MAX_PATH - fnamelen, L" - atto");
	SetConsoleTitleW(wndName);
}


bool attoFile_addNormalCh(atto_File * restrict file, wchar_t ch)
{
	atto_LNode * node = file->data.currentNode;

	if (node->freeSpaceLen < 1 && !attoLNode_realloc(node))
	{
		return false;
	}

	node->line[node->curx] = ch;
	++node->curx;
	--node->freeSpaceLen;
	return true;
}
bool attoFile_addSpecialCh(atto_File * restrict file, wchar_t ch)
{
	switch (ch)
	{
	case VK_TAB:
		for (int i = 0; i < 4; ++i)
		{
			if (attoFile_addNormalCh(file, ' ') == false)
			{
				return false;
			}
		}
		break;
	case VK_OEM_BACKTAB:
		// Check if there's 4 spaces before the caret
		if (attoFile_checkLineAt(file, -4, L"    ", 4))
		{
			for (int i = 0; i < 4; ++i)
			{
				attoFile_deleteBackward(file);
			}
		}
		// If there isn't, check if there's 4 spaces after the caret
		else if (attoFile_checkLineAt(file, 0, L"    ", 4))
		{
			for (int i = 0; i < 4; ++i)
			{
				attoFile_deleteForward(file);
			}
		}
		break;
	case VK_RETURN:	// Enter key
		attoFile_addNewLine(file);
		break;
	case VK_BACK:	// Backspace
		attoFile_deleteBackward(file);
		break;
	case VK_DELETE:	// Delete
		attoFile_deleteForward(file);
		break;
	// Move cursor
	case VK_LEFT:	// Left arrow
		if (file->data.currentNode->curx > 0)
		{
			attoLNode_moveCursor(file->data.currentNode, -1);
		}
		else if (file->data.currentNode->prevNode != NULL)
		{
			file->data.currentNode = file->data.currentNode->prevNode;
		}
		break;
	case VK_RIGHT:	// Right arrow
		if ((file->data.currentNode->curx + file->data.currentNode->freeSpaceLen) < file->data.currentNode->lineEndx)
		{
			attoLNode_moveCursor(file->data.currentNode, 1);
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

bool attoFile_checkLineAt(const atto_File * restrict file, int32_t maxdelta, const wchar_t * string, uint32_t maxString)
{
	const atto_LNode * restrict node = file->data.currentNode;
	if (node == NULL)
	{
		return false;
	}

	int32_t idx = (int32_t)node->curx + maxdelta, i = 0, m = (int32_t)maxString;
	if (idx < 0)
	{
		return false;
	}
	for (; idx < (int32_t)node->lineEndx && i < m && *string != '\0';)
	{
		if (idx == (int32_t)node->curx)
		{
			idx += (int32_t)node->freeSpaceLen;
			continue;
		}
		
		if (node->line[idx] != *string)
		{
			return false;
		}

		++string;
		++i;
		++idx;
	}
	if (*string != '\0' && i < m)
	{
		return false;
	}

	return true;
}
bool attoFile_deleteForward(atto_File * restrict file)
{
	atto_LNode * node = file->data.currentNode;
	if ((node->curx + node->freeSpaceLen) < node->lineEndx)
	{
		++node->freeSpaceLen;
		return true;
	}
	else if (node->nextNode != NULL)
	{
		return attoLNode_merge(node, &file->data.pcury);
	}
	else
	{
		return false;
	}
}
bool attoFile_deleteBackward(atto_File * restrict file)
{
	atto_LNode * node = file->data.currentNode;
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
		return attoLNode_merge(file->data.currentNode, &file->data.pcury);
	}
	else
	{
		return false;
	}
}
bool attoFile_addNewLine(atto_File * restrict file)
{
	atto_LNode * node = attoLNode_create(file->data.currentNode, file->data.currentNode->nextNode);
	if (node == NULL)
	{
		return false;
	}

	file->data.currentNode->nextNode = node;
	file->data.currentNode = node;
	return true;
}

void attoFile_updateCury(atto_File * restrict file, uint32_t height)
{
	if (file->data.pcury == NULL)
	{
		atto_LNode * node = file->data.currentNode;
		for (uint32_t i = 0; i < height && node->prevNode != NULL; ++i)
		{
			node = node->prevNode;
		}
		file->data.pcury = node;
	}
	else
	{
		atto_LNode * node = file->data.currentNode;
		for (uint32_t i = 0; i < height && node != NULL; ++i)
		{
			if (node == file->data.pcury)
			{
				return;
			}
			node = node->prevNode;
		}

		node = file->data.currentNode->nextNode;
		for (; node != NULL;)
		{
			if (node == file->data.pcury)
			{
				file->data.pcury = file->data.currentNode;
				return;
			}
			node = node->nextNode;
		}

		file->data.pcury = NULL;
		attoFile_updateCury(file, height);
	}
}


void attoFile_destruct(atto_File * restrict file)
{
	if (file->fileName != NULL && file->hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file->hFile);
		file->hFile = INVALID_HANDLE_VALUE;
	}
}


bool attoDS_init(atto_DS * restrict ds)
{
	ds->conIn  = GetStdHandle(STD_INPUT_HANDLE);
	ds->conOut = GetStdHandle(STD_OUTPUT_HANDLE);
	// Set exit handler
	atexit(&atto_exitHandler);

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
void attoDS_refresh(atto_DS * restrict ds)
{
	atto_updateScrbuf();
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		ds->scrbuf.handle,
		ds->scrbuf.mem,
		ds->scrbuf.w * (ds->scrbuf.h - 1),
		(COORD){ 0, 0 },
		&dwBytes
	);
}
void attoDS_refreshAll(atto_DS * restrict ds)
{
	atto_updateScrbuf();
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		ds->scrbuf.handle,
		ds->scrbuf.mem,
		ds->scrbuf.w * ds->scrbuf.h,
		(COORD){ 0, 0 },
		&dwBytes
	);
}
void attoDS_statusDraw(atto_DS * restrict ds, const wchar_t * message)
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
	attoDS_statusRefresh(ds);
}
void attoDS_statusRefresh(atto_DS * restrict ds)
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

void attoDS_destruct(atto_DS * restrict ds)
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
void atto_printErr(enum attoE errCode)
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
	atto_LNode * node = file.data.pcury;
	for (uint32_t i = 0; i < editor.scrbuf.h - 1; ++i)
	{
		if (node == NULL)
		{
			break;
		}

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
		*putf16 = malloc(sizeof(wchar_t));
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

