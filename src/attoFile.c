#include "attoFile.h"
#include "atto.h"
#include "profiling.h"

#include <stdlib.h>

attoLineNode * attoLine_create(attoLineNode * curnode, attoLineNode * nextnode)
{
	attoLineNode * node = malloc(sizeof(attoLineNode));
	if (node == NULL)
	{
		return NULL;
	}

	if (curnode != NULL)
	{
		// Normal empty line
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

attoLineNode * attoLine_createText(
	attoLineNode * curnode,
	attoLineNode * nextnode,
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

	attoLineNode * node = malloc(sizeof(attoLineNode));
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

bool attoLNode_getText(const attoLineNode * node, wchar_t ** text, uint32_t * tarrsz)
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

bool attoLNode_realloc(attoLineNode * restrict curnode)
{
	if (curnode->freeSpaceLen == ATTO_LNODE_DEFAULT_FREE)
	{
		return true;
	}
	uint32_t totalLen = curnode->lineEndx - curnode->freeSpaceLen;
	void * newmem = realloc(curnode->line, sizeof(wchar_t) * (totalLen + ATTO_LNODE_DEFAULT_FREE));
	if (newmem == NULL)
	{
		return false;
	}

	curnode->line = newmem;

	if (curnode->curx != totalLen)
	{
		memmove(
			curnode->line + curnode->curx + ATTO_LNODE_DEFAULT_FREE,
			curnode->line + curnode->curx + curnode->freeSpaceLen,
			sizeof(wchar_t) * (totalLen - curnode->curx - curnode->freeSpaceLen)
		);
	}

	curnode->lineEndx     = totalLen + ATTO_LNODE_DEFAULT_FREE;
	curnode->freeSpaceLen = ATTO_LNODE_DEFAULT_FREE;

	return true;
}

bool attoLNode_merge(attoLineNode * restrict node, attoLineNode ** restrict ppcury)
{
	if (node->nextNode == NULL)
	{
		return false;
	}
	
	attoLineNode * restrict n = node->nextNode;

	if (*ppcury == n)
	{
		*ppcury = node;
	}

	// Allocate more memory for first line
	void * linemem = realloc(node->line, sizeof(wchar_t) * (node->lineEndx - node->freeSpaceLen + n->lineEndx - n->freeSpaceLen + ATTO_LNODE_DEFAULT_FREE));
	if (linemem == NULL)
	{
		return false;
	}
	node->line = linemem;

	// Move cursor to end, if needed
	attoLNode_moveCursor(node, (int32_t)node->lineEndx);
	attoLNode_moveCursor(n, (int32_t)n->lineEndx);

	node->freeSpaceLen = ATTO_LNODE_DEFAULT_FREE;
	node->lineEndx = node->curx + n->curx + ATTO_LNODE_DEFAULT_FREE;

	memcpy(node->line + node->curx + node->freeSpaceLen, n->line, sizeof(wchar_t) * n->curx);
	node->nextNode = n->nextNode;
	if (node->nextNode != NULL)
	{
		node->nextNode->prevNode = node;
	}

	attoLNode_destroy(n); 

	return true;
}

void attoLNode_moveCursor(attoLineNode * restrict node, int32_t delta)
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

void attoLNode_destroy(attoLineNode * restrict node)
{
	if (node->line != NULL)
	{
		free(node->line);
		node->line = NULL;
	}
	free(node);
}

bool attoFile_open(attoFile * restrict file, const wchar_t * restrict fileName, bool writemode)
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
void attoFile_close(attoFile * restrict file)
{
	if (file->hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file->hFile);
		file->hFile = INVALID_HANDLE_VALUE;
	}
}
void attoFile_clearLines(attoFile * restrict file)
{
	attoLineNode * node = file->data.firstNode;
	file->data.firstNode = NULL;
	file->data.currentNode = NULL;
	file->data.pcury = NULL;
	if (node == NULL)
	{
		return;
	}
	while (node != NULL)
	{
		attoLineNode * next = node->nextNode;
		attoLNode_destroy(node);
		node = next;
	}
}
const wchar_t * attoFile_read(attoFile * restrict file)
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
		file->data.firstNode = attoLine_create(NULL, NULL);
		if (file->data.firstNode == NULL)
		{
			free(lines);
			free(utf16);
			return L"Line creation error!";
		}
	}
	else
	{
		file->data.firstNode = attoLine_createText(NULL, NULL, lines[0], -1);
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
		attoLineNode * node = attoLine_createText(file->data.currentNode, NULL, lines[i], -1);
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
int attoFile_write(attoFile * restrict file)
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

	attoLineNode * node = file->data.firstNode;

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
void attoFile_setConTitle(attoFile * restrict file)
{
	wchar_t wndName[MAX_PATH];
	size_t fnamelen = wcslen(file->fileName);
	memcpy(wndName, file->fileName, fnamelen * sizeof(wchar_t));
	wcscpy_s(wndName + fnamelen, MAX_PATH - fnamelen, L" - atto");
	SetConsoleTitleW(wndName);
}


bool attoFile_addNormalCh(attoFile * restrict file, wchar_t ch)
{
	attoLineNode * node = file->data.currentNode;

	if (node->freeSpaceLen < 1 && !attoLNode_realloc(node))
	{
		return false;
	}

	node->line[node->curx] = ch;
	++node->curx;
	--node->freeSpaceLen;
	return true;
}
bool attoFile_addSpecialCh(attoFile * restrict file, wchar_t ch)
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

bool attoFile_checkLineAt(const attoFile * restrict file, int32_t maxdelta, const wchar_t * string, uint32_t maxString)
{
	const attoLineNode * restrict node = file->data.currentNode;
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
bool attoFile_deleteForward(attoFile * restrict file)
{
	attoLineNode * node = file->data.currentNode;
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
bool attoFile_deleteBackward(attoFile * restrict file)
{
	attoLineNode * node = file->data.currentNode;
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
bool attoFile_addNewLine(attoFile * restrict file)
{
	attoLineNode * node = attoLine_create(file->data.currentNode, file->data.currentNode->nextNode);
	if (node == NULL)
	{
		return false;
	}

	file->data.currentNode->nextNode = node;
	file->data.currentNode = node;
	return true;
}

void attoFile_updateCury(attoFile * restrict file, uint32_t height)
{
	if (file->data.pcury == NULL)
	{
		attoLineNode * node = file->data.currentNode;
		for (uint32_t i = 0; i < height && node->prevNode != NULL; ++i)
		{
			node = node->prevNode;
		}
		file->data.pcury = node;
	}
	else
	{
		attoLineNode * node = file->data.currentNode;
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


void attoFile_destruct(attoFile * restrict file)
{
	if (file->fileName != NULL && file->hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file->hFile);
		file->hFile = INVALID_HANDLE_VALUE;
	}
}