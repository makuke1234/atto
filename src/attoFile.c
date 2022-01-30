#include "attoFile.h"
#include "atto.h"
#include "profiling.h"

#include <stdlib.h>

attoLineNode_t * attoLine_create(attoLineNode_t * restrict curnode, attoLineNode_t * restrict nextnode)
{
	attoLineNode_t * node = malloc(sizeof(attoLineNode_t));
	if (node == NULL)
	{
		return NULL;
	}

	if (curnode != NULL)
	{
		// Create normal empty line
		if ((curnode->curx + curnode->freeSpaceLen) == curnode->lineEndx)
		{
			node->line = malloc(sizeof(wchar_t) * ATTO_LNODE_DEFAULT_FREE);
			if (node->line == NULL)
			{
				free(node);
				return NULL;
			}
			node->lineEndx = ATTO_LNODE_DEFAULT_FREE;
		}
		// Copy contents after cursor to this line
		else
		{
			uint32_t contStart = curnode->curx + curnode->freeSpaceLen, contLen = curnode->lineEndx - contStart;
			node->lineEndx = contLen + ATTO_LNODE_DEFAULT_FREE;
			node->line = malloc(sizeof(wchar_t) * node->lineEndx);
			if (node->line == NULL)
			{
				free(node);
				return NULL;
			}
			memcpy(node->line + ATTO_LNODE_DEFAULT_FREE, curnode->line + contStart, sizeof(wchar_t) * contLen);
			curnode->freeSpaceLen += contLen;
		}
	}

	node->curx = 0;
	node->freeSpaceLen = ATTO_LNODE_DEFAULT_FREE;
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

attoLineNode_t * attoLine_createText(
	attoLineNode_t * restrict curnode,
	attoLineNode_t * restrict nextnode,
	const wchar_t * restrict lineText,
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

	attoLineNode_t * node = malloc(sizeof(attoLineNode_t));
	if (node == NULL)
	{
		return NULL;
	}

	node->lineEndx = maxText + ATTO_LNODE_DEFAULT_FREE;
	node->line = malloc(sizeof(wchar_t) * node->lineEndx);
	if (node->line == NULL)
	{
		free(node);
		return NULL;
	}
	
	memcpy(node->line, lineText, sizeof(wchar_t) * maxText);

	node->curx = maxText;
	node->freeSpaceLen = ATTO_LNODE_DEFAULT_FREE;

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

bool attoLine_getText(const attoLineNode_t * restrict self, wchar_t ** restrict text, uint32_t * restrict tarrsz)
{
	if (text == NULL)
	{
		return false;
	}
	uint32_t totalLen = self->lineEndx - self->freeSpaceLen + 1;

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
	for (uint32_t i = 0; i < self->lineEndx;)
	{
		if (i == self->curx)
		{
			i += self->freeSpaceLen;
			continue;
		}

		*t = self->line[i];
		++t;
		++i;
	}
	*t = L'\0';

	return true;
}

bool attoLine_realloc(attoLineNode_t * restrict self)
{
	if (self->freeSpaceLen == ATTO_LNODE_DEFAULT_FREE)
	{
		return true;
	}
	uint32_t totalLen = self->lineEndx - self->freeSpaceLen;
	void * newmem = realloc(self->line, sizeof(wchar_t) * (totalLen + ATTO_LNODE_DEFAULT_FREE));
	if (newmem == NULL)
	{
		return false;
	}
	self->line = newmem;
	if (self->curx != totalLen)
	{
		memmove(
			self->line + self->curx + ATTO_LNODE_DEFAULT_FREE,
			self->line + self->curx + self->freeSpaceLen,
			sizeof(wchar_t) * (totalLen - self->curx - self->freeSpaceLen)
		);
	}

	self->lineEndx     = totalLen + ATTO_LNODE_DEFAULT_FREE;
	self->freeSpaceLen = ATTO_LNODE_DEFAULT_FREE;

	return true;
}

bool attoLine_mergeNext(attoLineNode_t * restrict self, attoLineNode_t ** restrict ppcury)
{
	if (self->nextNode == NULL)
	{
		return false;
	}
	
	attoLineNode_t * restrict n = self->nextNode;

	if (*ppcury == n)
	{
		*ppcury = self;
	}

	// Allocate more memory for first line
	void * linemem = realloc(
		self->line,
		sizeof(wchar_t) * (self->lineEndx - self->freeSpaceLen + n->lineEndx - n->freeSpaceLen + ATTO_LNODE_DEFAULT_FREE)
	);
	if (linemem == NULL)
	{
		return false;
	}
	self->line = linemem;

	// Move cursor to end, if needed
	attoLine_moveCursor(self, (int32_t)self->lineEndx);
	attoLine_moveCursor(n,    (int32_t)n->lineEndx);

	self->freeSpaceLen = ATTO_LNODE_DEFAULT_FREE;
	self->lineEndx     = self->curx + n->curx + ATTO_LNODE_DEFAULT_FREE;

	memcpy(self->line + self->curx + self->freeSpaceLen, n->line, sizeof(wchar_t) * n->curx);
	self->nextNode = n->nextNode;
	if (self->nextNode != NULL)
	{
		self->nextNode->prevNode = self;
	}
	attoLine_destroy(n); 

	return true;
}

void attoLine_moveCursor(attoLineNode_t * restrict self, int32_t delta)
{
	if (delta < 0)
	{
		for (uint32_t idx = self->curx + self->freeSpaceLen; delta < 0 && self->curx > 0; ++delta)
		{
			--idx;
			--self->curx;
			self->line[idx] = self->line[self->curx];
		}
	}
	else
	{
		for (uint32_t total = self->lineEndx - self->freeSpaceLen, idx = self->curx + self->freeSpaceLen; delta > 0 && self->curx < total; --delta)
		{
			self->line[self->curx] = self->line[idx];
			++idx;
			++self->curx;
		}
	}
}

void attoLine_destroy(attoLineNode_t * restrict self)
{
	if (self->line != NULL)
	{
		free(self->line);
		self->line = NULL;
	}
	free(self);
}

void attoFile_reset(attoFile_t * restrict self)
{
	(*self) = (attoFile_t){
		.fileName = NULL,
		.hFile    = INVALID_HANDLE_VALUE,
		.canWrite = false,
		.data     = {
			.firstNode   = NULL,
			.currentNode = NULL,
			.pcury       = NULL,
			.curx        = 0
		}
	};
}
bool attoFile_open(attoFile_t * restrict self, const wchar_t * restrict fileName, bool writemode)
{
	if (fileName == NULL)
	{
		fileName = self->fileName;
	}

	// try to open file
	self->hFile = CreateFileW(
		fileName,
		writemode ? GENERIC_WRITE : GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		writemode ? CREATE_ALWAYS : OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (self->hFile == INVALID_HANDLE_VALUE)
	{
		self->canWrite = false;
		return false;
	}
	else
	{
		self->canWrite = writemode;
		self->fileName = fileName;
		return true;
	}
}
void attoFile_close(attoFile_t * restrict self)
{
	if (self->hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(self->hFile);
		self->hFile = INVALID_HANDLE_VALUE;
	}
}
void attoFile_clearLines(attoFile_t * restrict self)
{
	attoLineNode_t * node  = self->data.firstNode;
	self->data.firstNode   = NULL;
	self->data.currentNode = NULL;
	self->data.pcury       = NULL;
	if (node == NULL)
	{
		return;
	}
	while (node != NULL)
	{
		attoLineNode_t * next = node->nextNode;
		attoLine_destroy(node);
		node = next;
	}
}
const wchar_t * attoFile_read(attoFile_t * restrict self)
{
	if (attoFile_open(self, NULL, false) == false)
	{
		return L"File opening error!\n";
	}

	// Clear lines
	attoFile_clearLines(self);

	// Get file size
	DWORD fileSize = GetFileSize(self->hFile, NULL);
	writeProfiler("attoFile_read", "Opened file with size of %u bytes", fileSize);

	// Alloc array
	char * bytes = malloc(fileSize + 1);
	if (bytes == NULL)
	{
		return L"Memory error!";
	}
	BOOL readFileRes = ReadFile(
		self->hFile,
		bytes,
		fileSize,
		NULL,
		NULL
	);
	attoFile_close(self);

	if (!readFileRes)
	{
		free(bytes);
		return L"File read error!";
	}
	bytes[fileSize] = '\0';
	++fileSize;

	// Convert to UTF-16
	wchar_t * utf16 = NULL;
	uint32_t chars = atto_convToUnicode(bytes, (int)fileSize, &utf16, NULL);
	free(bytes);

	if (utf16 == NULL)
	{
		return L"Unicode conversion error!";
	}
	writeProfiler("attoFile_read", "Converted %u bytes of character to %u UTF-16 characters.", fileSize, chars);
	writeProfiler("attoFile_read", "File UTF-16 contents \"%S\"", utf16);

	// Convert tabs to spaces
	atto_tabsToSpaces(&utf16, &chars);

	// Save lines to structure
	wchar_t ** lines = NULL;
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

	attoFile_clearLines(self);
	if (numLines == 0)
	{
		self->data.firstNode = attoLine_create(NULL, NULL);
		if (self->data.firstNode == NULL)
		{
			free(lines);
			free(utf16);
			return L"Line creation error!";
		}
	}
	else
	{
		self->data.firstNode = attoLine_createText(NULL, NULL, lines[0], -1);
		if (self->data.firstNode == NULL)
		{
			free(lines);
			free(utf16);
			return L"Line creation error!";
		}
	}
	self->data.currentNode = self->data.firstNode;
	for (uint32_t i = 1; i < numLines; ++i)
	{
		attoLineNode_t * node = attoLine_createText(self->data.currentNode, NULL, lines[i], -1);
		if (node == NULL)
		{
			free(lines);
			free(utf16);
			return L"Line creation error!";
		}
		self->data.currentNode = node;
	}
	free(lines);
	free(utf16);

	return NULL;
}
int attoFile_write(attoFile_t * restrict self)
{
	if (attoFile_open(self, NULL, true) == false)
	{
		return writeRes_openError;
	}

	if (self->canWrite == false)
	{
		attoFile_close(self);
		return writeRes_writeError;
	}

	writeProfiler("attoFile_write", "Opened file for writing");

	// Save to file here

	wchar_t * lines = NULL, * line = NULL;
	uint32_t linesCap = 0, linesLen = 0, lineCap = 0;

	attoLineNode_t * node = self->data.firstNode;

	while (node != NULL)
	{
		if (attoLine_getText(node, &line, &lineCap) == false)
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

	// Error-check conversion
	if (utf8 == NULL)
	{
		attoFile_close(self);
		return writeRes_memError;
	}

	writeProfiler("attoFile_write", "Converted UTF-16 string to UTF-8 string");
	writeProfiler("attoFile_write", "UTF-8 contents: \"%s\"", utf8);

	// Try to write UTF-8 lines string to file
	DWORD dwWritten;
	BOOL res = WriteFile(
		self->hFile,
		utf8,
		utf8sz,
		&dwWritten,
		NULL
	);

	// Close file
	attoFile_close(self);
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
void attoFile_setConTitle(const attoFile_t * restrict self)
{
	wchar_t wndName[MAX_PATH];
	size_t fnamelen = wcslen(self->fileName);
	memcpy(wndName, self->fileName, fnamelen * sizeof(wchar_t));
	wcscpy_s(wndName + fnamelen, MAX_PATH - fnamelen, L" - atto");
	SetConsoleTitleW(wndName);
}


bool attoFile_addNormalCh(attoFile_t * restrict self, wchar_t ch)
{
	attoLineNode_t * node = self->data.currentNode;

	if (node->freeSpaceLen < 1 && !attoLine_realloc(node))
	{
		return false;
	}

	node->line[node->curx] = ch;
	++node->curx;
	--node->freeSpaceLen;
	return true;
}
bool attoFile_addSpecialCh(attoFile_t * restrict self, wchar_t ch)
{
	switch (ch)
	{
	case VK_TAB:
		for (int i = 0; i < 4; ++i)
		{
			if (attoFile_addNormalCh(self, ' ') == false)
			{
				return false;
			}
		}
		break;
	case VK_OEM_BACKTAB:
		// Check if there's 4 spaces before the caret
		if (attoFile_checkLineAt(self, -4, L"    ", 4))
		{
			for (int i = 0; i < 4; ++i)
			{
				attoFile_deleteBackward(self);
			}
		}
		// If there isn't, check if there's 4 spaces after the caret
		else if (attoFile_checkLineAt(self, 0, L"    ", 4))
		{
			for (int i = 0; i < 4; ++i)
			{
				attoFile_deleteForward(self);
			}
		}
		break;
	case VK_RETURN:	// Enter key
		attoFile_addNewLine(self);
		break;
	case VK_BACK:	// Backspace
		attoFile_deleteBackward(self);
		break;
	case VK_DELETE:	// Delete
		attoFile_deleteForward(self);
		break;
	// Move cursor
	case VK_LEFT:	// Left arrow
		if (self->data.currentNode->curx > 0)
		{
			attoLine_moveCursor(self->data.currentNode, -1);
		}
		else if (self->data.currentNode->prevNode != NULL)
		{
			self->data.currentNode = self->data.currentNode->prevNode;
		}
		break;
	case VK_RIGHT:	// Right arrow
		if ((self->data.currentNode->curx + self->data.currentNode->freeSpaceLen) < self->data.currentNode->lineEndx)
		{
			attoLine_moveCursor(self->data.currentNode, 1);
		}
		else if (self->data.currentNode->nextNode != NULL)
		{
			self->data.currentNode = self->data.currentNode->nextNode;
		}
		break;
	case VK_UP:		// Up arrow
		if (self->data.currentNode->prevNode != NULL)
		{
			self->data.currentNode = self->data.currentNode->prevNode;
		}
		break;
	case VK_DOWN:	// Down arrow
		if (self->data.currentNode->nextNode != NULL)
		{
			self->data.currentNode = self->data.currentNode->nextNode;
		}
		break;
	default:
		return false;
	}

	return true;
}

bool attoFile_checkLineAt(const attoFile_t * restrict self, int32_t maxdelta, const wchar_t * restrict string, uint32_t maxString)
{
	const attoLineNode_t * restrict node = self->data.currentNode;
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
bool attoFile_deleteForward(attoFile_t * restrict self)
{
	attoLineNode_t * node = self->data.currentNode;
	if ((node->curx + node->freeSpaceLen) < node->lineEndx)
	{
		++node->freeSpaceLen;
		return true;
	}
	else if (node->nextNode != NULL)
	{
		return attoLine_mergeNext(node, &self->data.pcury);
	}
	else
	{
		return false;
	}
}
bool attoFile_deleteBackward(attoFile_t * restrict self)
{
	attoLineNode_t * node = self->data.currentNode;
	if (node->curx > 0)
	{
		--node->curx;
		++node->freeSpaceLen;
		return true;
	}
	else if (node->prevNode != NULL)
	{
		// Add current node data to previous node data
		self->data.currentNode = node->prevNode;
		return attoLine_mergeNext(self->data.currentNode, &self->data.pcury);
	}
	else
	{
		return false;
	}
}
bool attoFile_addNewLine(attoFile_t * restrict self)
{
	attoLineNode_t * node = attoLine_create(self->data.currentNode, self->data.currentNode->nextNode);
	if (node == NULL)
	{
		return false;
	}

	self->data.currentNode->nextNode = node;
	self->data.currentNode = node;
	return true;
}

void attoFile_updateCury(attoFile_t * restrict self, uint32_t height)
{
	if (self->data.pcury == NULL)
	{
		attoLineNode_t * node = self->data.currentNode;
		for (uint32_t i = 0; i < height && node->prevNode != NULL; ++i)
		{
			node = node->prevNode;
		}
		self->data.pcury = node;
	}
	else
	{
		attoLineNode_t * node = self->data.currentNode;
		for (uint32_t i = 0; i < height && node != NULL; ++i)
		{
			if (node == self->data.pcury)
			{
				return;
			}
			node = node->prevNode;
		}

		node = self->data.currentNode->nextNode;
		for (; node != NULL;)
		{
			if (node == self->data.pcury)
			{
				self->data.pcury = self->data.currentNode;
				return;
			}
			node = node->nextNode;
		}

		self->data.pcury = NULL;
		attoFile_updateCury(self, height);
	}
}


void attoFile_destruct(attoFile_t * restrict self)
{
	attoFile_close(self);
	attoFile_clearLines(self);
}
