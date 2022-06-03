#include "aFile.h"
#include "atto.h"


aLine_t * aLine_create(aLine_t * restrict curnode, aLine_t * restrict nextnode)
{
	aLine_t * node = malloc(sizeof(aLine_t));
	if (node == NULL)
	{
		return NULL;
	}

	if (curnode != NULL)
	{
		// Create normal empty line
		if ((curnode->curx + curnode->freeSpaceLen) == curnode->lineEndx)
		{
			node->line = malloc(sizeof(wchar) * ATTO_LNODE_DEFAULT_FREE);
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
			const usize contStart = curnode->curx + curnode->freeSpaceLen, contLen = curnode->lineEndx - contStart;
			node->lineEndx = contLen + ATTO_LNODE_DEFAULT_FREE;
			node->line = malloc(sizeof(wchar) * node->lineEndx);
			if (node->line == NULL)
			{
				free(node);
				return NULL;
			}
			memcpy(node->line + ATTO_LNODE_DEFAULT_FREE, curnode->line + contStart, sizeof(wchar) * contLen);
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

aLine_t * aLine_createText(
	aLine_t * restrict curnode,
	aLine_t * restrict nextnode,
	const wchar * restrict lineText,
	isize mText
)
{
	const usize maxText = mText == -1 ? wcslen(lineText) : (usize)mText;

	aLine_t * node = malloc(sizeof(aLine_t));
	if (node == NULL)
	{
		return NULL;
	}

	node->lineEndx = maxText + ATTO_LNODE_DEFAULT_FREE;
	node->line = malloc(sizeof(wchar) * node->lineEndx);
	if (node->line == NULL)
	{
		free(node);
		return NULL;
	}
	
	memcpy(node->line, lineText, sizeof(wchar) * maxText);

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

bool aLine_getText(const aLine_t * restrict self, wchar ** restrict text, usize * restrict tarrsz)
{
	const usize totalLen = self->lineEndx - self->freeSpaceLen + 1;

	if ((tarrsz != NULL) && (*tarrsz < totalLen))
	{
		wchar * mem = realloc(*text, sizeof(wchar) * totalLen);
		if (mem == NULL)
		{
			return false;
		}
		*text   = mem;
		*tarrsz = totalLen;
	}
	else if (tarrsz == NULL)
	{
		*text = malloc(sizeof(wchar) * totalLen);
		if (*text == NULL)
		{
			return false;
		}
	}

	wchar * restrict t = *text;
	for (usize i = 0; i < self->lineEndx;)
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

bool aLine_realloc(aLine_t * restrict self)
{
	if (self->freeSpaceLen == ATTO_LNODE_DEFAULT_FREE)
	{
		return true;
	}
	const usize totalLen = self->lineEndx - self->freeSpaceLen;
	vptr newmem = realloc(self->line, sizeof(wchar) * (totalLen + ATTO_LNODE_DEFAULT_FREE));
	if (newmem == NULL)
	{
		return false;
	}
	self->line = newmem;
	if (self->curx < totalLen)
	{
		memmove(
			self->line + self->curx + ATTO_LNODE_DEFAULT_FREE,
			self->line + self->curx + self->freeSpaceLen,
			sizeof(wchar) * (totalLen - self->curx)
		);
	}

	self->lineEndx     = totalLen + ATTO_LNODE_DEFAULT_FREE;
	self->freeSpaceLen = ATTO_LNODE_DEFAULT_FREE;

	return true;
}

bool aLine_mergeNext(aLine_t * restrict self, aLine_t ** restrict ppcury)
{
	if (self->nextNode == NULL)
	{
		return false;
	}
	
	aLine_t * restrict n = self->nextNode;
	*ppcury = (*ppcury == n) ? self : *ppcury;

	// Allocate more memory for first line
	vptr linemem = realloc(
		self->line,
		sizeof(wchar) * (self->lineEndx - self->freeSpaceLen + n->lineEndx - n->freeSpaceLen + ATTO_LNODE_DEFAULT_FREE)
	);
	if (linemem == NULL)
	{
		return false;
	}
	self->line = linemem;

	// Move cursor to end, if needed
	aLine_moveCursor(self, (isize)self->lineEndx);
	aLine_moveCursor(n,    (isize)n->lineEndx);

	self->freeSpaceLen = ATTO_LNODE_DEFAULT_FREE;
	self->lineEndx     = self->curx + n->curx + ATTO_LNODE_DEFAULT_FREE;

	memcpy(self->line + self->curx + self->freeSpaceLen, n->line, sizeof(wchar) * n->curx);
	self->nextNode = n->nextNode;
	if (self->nextNode != NULL)
	{
		self->nextNode->prevNode = self;
	}
	aLine_destroy(n); 

	return true;
}

void aLine_moveCursor(aLine_t * restrict self, isize delta)
{
	if (delta < 0)
	{
		for (usize idx = self->curx + self->freeSpaceLen; delta < 0 && self->curx > 0; ++delta)
		{
			--idx;
			--self->curx;
			self->line[idx] = self->line[self->curx];
		}
	}
	else
	{
		for (usize total = self->lineEndx - self->freeSpaceLen, idx = self->curx + self->freeSpaceLen; delta > 0 && self->curx < total; --delta)
		{
			self->line[self->curx] = self->line[idx];
			++idx;
			++self->curx;
		}
	}
}

void aLine_destroy(aLine_t * restrict self)
{
	if (self->line != NULL)
	{
		free(self->line);
		self->line = NULL;
	}
	free(self);
}

void aFile_reset(aFile_t * restrict self)
{
	(*self) = (aFile_t){
		.fileName = NULL,
		.hFile    = INVALID_HANDLE_VALUE,
		.canWrite = false,
		.eolSeq   = eolNOT,
		.data     = {
			.firstNode   = NULL,
			.currentNode = NULL,
			.pcury       = NULL,
			.curx        = 0
		}
	};
}
bool aFile_open(aFile_t * restrict self, const wchar * restrict fileName, bool writemode)
{
	fileName = (fileName == NULL) ? self->fileName : fileName;

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
void aFile_close(aFile_t * restrict self)
{
	if (self->hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(self->hFile);
		self->hFile = INVALID_HANDLE_VALUE;
	}
}
void aFile_clearLines(aFile_t * restrict self)
{
	aLine_t * restrict node  = self->data.firstNode;
	self->data.firstNode   = NULL;
	self->data.currentNode = NULL;
	self->data.pcury       = NULL;
	while (node != NULL)
	{
		aLine_t * restrict next = node->nextNode;
		aLine_destroy(node);
		node = next;
	}
}
const wchar * aFile_readBytes(aFile_t * restrict self, char ** restrict bytes, usize * restrict bytesLen)
{
	if (aFile_open(self, NULL, false) == false)
	{
		return L"File opening error!";
	}
	DWORD fileSize = GetFileSize(self->hFile, NULL);
	if ((fileSize >= *bytesLen) || (*bytes == NULL))
	{
		vptr mem = realloc(*bytes, fileSize + 1);
		if (mem == NULL)
		{
			return L"Memory error!";
		}
		*bytes    = mem;
		*bytesLen = fileSize + 1;
	}

	BOOL readFileRes = ReadFile(
		self->hFile,
		*bytes,
		fileSize,
		NULL,
		NULL
	);
	aFile_close(self);
	if (!readFileRes)
	{
		return L"File read error!";
	}
	// Add null terminator
	(*bytes)[fileSize] = '\0';

	return NULL;
}
const wchar * aFile_read(aFile_t * restrict self)
{
	char * bytes = NULL;
	usize size;
	const wchar * res;
	if ((res = aFile_readBytes(self, &bytes, &size)) != NULL)
	{
		return res;
	}

	// Convert to UTF-16
	wchar * utf16 = NULL;
	usize chars = (usize)atto_toutf16(bytes, (int)size, &utf16, NULL);
	free(bytes);

	if (utf16 == NULL)
	{
		return L"Unicode conversion error!";
	}

	// Convert tabs to spaces
	atto_tabsToSpaces(&utf16, &chars);

	// Save lines to structure
	wchar ** lines = NULL;
	const usize numLines = atto_strnToLines(utf16, chars, &lines, &self->eolSeq);
	if (lines == NULL)
	{
		free(utf16);
		return L"Line reading error!";
	}

	aFile_clearLines(self);
	if (numLines == 0)
	{
		self->data.firstNode = aLine_create(NULL, NULL);
		if (self->data.firstNode == NULL)
		{
			free(lines);
			free(utf16);
			return L"Line creation error!";
		}
	}
	else
	{
		self->data.firstNode = aLine_createText(NULL, NULL, lines[0], -1);
		if (self->data.firstNode == NULL)
		{
			free(lines);
			free(utf16);
			return L"Line creation error!";
		}
	}
	self->data.currentNode = self->data.firstNode;
	for (usize i = 1; i < numLines; ++i)
	{
		aLine_t * node = aLine_createText(self->data.currentNode, NULL, lines[i], -1);
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
isize aFile_write(aFile_t * restrict self)
{
	// Generate lines
	wchar * lines = NULL, * line = NULL;
	usize linesCap = 0, linesLen = 0, lineCap = 0;

	const aLine_t * node = self->data.firstNode;

	const eolSeq_e eolSeq = self->eolSeq;
	const bool isCRLF = (self->eolSeq == eolCRLF);

	while (node != NULL)
	{
		if (aLine_getText(node, &line, &lineCap) == false)
		{
			if (line != NULL)
			{
				free(line);
			}
			if (lines != NULL)
			{
				free(lines);
			}
			return afwrMEM_ERROR;
		}

		const usize lineLen = wcsnlen(line, lineCap);
		const usize addnewline = (node->nextNode != NULL) ? 1 + (usize)isCRLF : 0;
		const usize newLinesLen = linesLen + lineLen + addnewline;

		// Add line to lines, concatenate \n character, if necessary

		if (newLinesLen >= linesCap)
		{
			// Resize lines array
			const usize newCap = (newLinesLen + 1) * 2;

			vptr mem = realloc(lines, sizeof(wchar) * newCap);
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
				return afwrMEM_ERROR;
			}

			lines    = mem;
			linesCap = newCap;
		}

		// Copy line
		memcpy(lines + linesLen, line, sizeof(wchar) * lineLen);
		linesLen = newLinesLen;

		if (addnewline)
		{
			switch (eolSeq)
			{
			case eolCR:
				lines[linesLen - 1] = L'\r';
				break;
			case eolLF:
				lines[linesLen - 1] = L'\n';
				break;
			case eolCRLF:
				lines[linesLen - 2] = L'\r';
				lines[linesLen - 1] = L'\n';
				break;
			case eolNOT:
				assert(!"EOL sequence not selected!");
				break;
			}
		}
		lines[linesLen] = L'\0';

		node = node->nextNode;
	}
	free(line);

	// Try to convert lines string to UTF-8
	char * utf8 = NULL;
	usize utf8sz = 0;
	atto_toutf8(lines, (int)linesLen + 1, &utf8, &utf8sz);

	// Free UTF-16 lines string
	free(lines);

	// Error-check conversion
	if (utf8 == NULL)
	{
		return afwrMEM_ERROR;
	}

	// Check if anything has changed, for that load original file again
	char * compFile = NULL;
	usize compSize;
	if (aFile_readBytes(self, &compFile, &compSize) == NULL)
	{
		// Reading was successful
		bool areEqual = strncmp(utf8, compFile, min_usize(utf8sz, compSize)) == 0;
		free(compFile);

		if (areEqual)
		{
			// Free all resources before returning
			free(utf8);
			return afwrNOTHING_NEW;
		}
	}

	// Try to open file for writing
	if (aFile_open(self, NULL, true) == false)
	{
		return afwrOPEN_ERROR;
	}

	if (self->canWrite == false)
	{
		aFile_close(self);
		return afwrWRITE_ERROR;
	}

	// Try to write UTF-8 lines string to file
	DWORD dwWritten;
	// Write everything except the null terminator
	const BOOL res = WriteFile(
		self->hFile,
		utf8,
		(DWORD)(utf8sz - (usize)1),
		&dwWritten,
		NULL
	);
	// Close file
	aFile_close(self);
	// Free utf8 string
	free(utf8);

	// Do error checking
	if (!res)
	{
		return afwrWRITE_ERROR;
	}
	else
	{
		return (i32)dwWritten;
	}
}
void aFile_setConTitle(const aFile_t * restrict self)
{
	wchar wndName[MAX_PATH];
	const usize fnamelen = min_usize(MAX_PATH - 1, wcslen(self->fileName));
	memcpy(wndName, self->fileName, fnamelen * sizeof(wchar));
	wcscpy_s(wndName + fnamelen, MAX_PATH - fnamelen, L" - atto");
	SetConsoleTitleW(wndName);
}


bool aFile_addNormalCh(aFile_t * restrict self, wchar ch)
{
	aLine_t * restrict node = self->data.currentNode;
	if ((node->freeSpaceLen == 0) && !aLine_realloc(node))
	{
		return false;
	}

	node->line[node->curx] = ch;
	++node->curx;
	--node->freeSpaceLen;
	return true;
}
bool aFile_addSpecialCh(aFile_t * restrict self, wchar ch)
{
	switch (ch)
	{
	case VK_TAB:
		for (u8 i = 0; i < 4; ++i)
		{
			if (aFile_addNormalCh(self, ' ') == false)
			{
				return false;
			}
		}
		break;
	case VK_OEM_BACKTAB:
		// Check if there's 4 spaces before the caret
		if (aFile_checkLineAt(self, -4, L"    ", 4))
		{
			for (u8 i = 0; i < 4; ++i)
			{
				aFile_deleteBackward(self);
			}
		}
		// If there isn't, check if there's 4 spaces after the caret
		else if (aFile_checkLineAt(self, 0, L"    ", 4))
		{
			for (int i = 0; i < 4; ++i)
			{
				aFile_deleteForward(self);
			}
		}
		break;
	case VK_RETURN:	// Enter key
		aFile_addNewLine(self);
		break;
	case VK_BACK:	// Backspace
		aFile_deleteBackward(self);
		break;
	case VK_DELETE:	// Delete
		aFile_deleteForward(self);
		break;
	// Move cursor
	case VK_LEFT:	// Left arrow
		if (self->data.currentNode->curx > 0)
		{
			aLine_moveCursor(self->data.currentNode, -1);
		}
		else if (self->data.currentNode->prevNode != NULL)
		{
			self->data.currentNode = self->data.currentNode->prevNode;
		}
		break;
	case VK_RIGHT:	// Right arrow
		if ((self->data.currentNode->curx + self->data.currentNode->freeSpaceLen) < self->data.currentNode->lineEndx)
		{
			aLine_moveCursor(self->data.currentNode, 1);
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

bool aFile_checkLineAt(const aFile_t * restrict self, isize maxdelta, const wchar * restrict string, usize maxString)
{
	const aLine_t * restrict node = self->data.currentNode;
	if (node == NULL)
	{
		return false;
	}

	isize idx = (isize)node->curx + maxdelta, i = 0;
	const isize m = (isize)maxString;
	if (idx < 0)
	{
		return false;
	}
	for (; idx < (isize)node->lineEndx && i < m && *string != '\0';)
	{
		if ((idx == (isize)node->curx) && (node->freeSpaceLen > 0))
		{
			idx += (isize)node->freeSpaceLen;
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
bool aFile_deleteForward(aFile_t * restrict self)
{
	aLine_t * restrict node = self->data.currentNode;
	if ((node->curx + node->freeSpaceLen) < node->lineEndx)
	{
		++node->freeSpaceLen;
		return true;
	}
	else if (node->nextNode != NULL)
	{
		return aLine_mergeNext(node, &self->data.pcury);
	}
	else
	{
		return false;
	}
}
bool aFile_deleteBackward(aFile_t * restrict self)
{
	aLine_t * restrict node = self->data.currentNode;
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
		return aLine_mergeNext(self->data.currentNode, &self->data.pcury);
	}
	else
	{
		return false;
	}
}
bool aFile_addNewLine(aFile_t * restrict self)
{
	aLine_t * restrict node = aLine_create(self->data.currentNode, self->data.currentNode->nextNode);
	if (node == NULL)
	{
		return false;
	}

	self->data.currentNode->nextNode = node;
	self->data.currentNode = node;
	return true;
}

void aFile_updateCury(aFile_t * restrict self, u32 height)
{
	if (self->data.pcury == NULL)
	{
		aLine_t * restrict node = self->data.currentNode;
		for (u32 i = 0; (i < height) && (node->prevNode != NULL); ++i)
		{
			node = node->prevNode;
		}
		self->data.pcury = node;
	}
	else
	{
		aLine_t * restrict node = self->data.currentNode;
		for (u32 i = 0; (i < height) && (node != NULL); ++i)
		{
			if (node == self->data.pcury)
			{
				return;
			}
			node = node->prevNode;
		}

		node = self->data.currentNode->nextNode;
		while (node != NULL)
		{
			if (node == self->data.pcury)
			{
				self->data.pcury = self->data.currentNode;
				return;
			}
			node = node->nextNode;
		}

		self->data.pcury = NULL;
		aFile_updateCury(self, height);
	}
}


void aFile_destroy(aFile_t * restrict self)
{
	aFile_close(self);
	aFile_clearLines(self);
}
