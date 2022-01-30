#ifndef ATTOFILE_H
#define ATTOFILE_H

#include "common.h"

#include <stdint.h>
#include <stdbool.h>

#define ATTO_LNODE_DEFAULT_FREE 10

/*
	Example:
	L"This is text\0\0\0\0\0\0"
	              ^ - curx
				  <----------> - freeSpaceLen = 6
*/

typedef struct attoLineNode
{
	wchar_t * line;
	uint32_t lineEndx, curx, freeSpaceLen;
	struct attoLineNode * prevNode, * nextNode;
} attoLineNode;

attoLineNode * attoLine_create(attoLineNode * curnode, attoLineNode * nextnode);
attoLineNode * attoLine_createText(
	attoLineNode * curnode,
	attoLineNode * nextnode,
	const wchar_t * lineText,
	int32_t maxText
);

bool attoLNode_getText(const attoLineNode * node, wchar_t ** text, uint32_t * tarrsz);

bool attoLNode_realloc(attoLineNode * restrict curnode);

bool attoLNode_merge(attoLineNode * restrict node, attoLineNode ** restrict ppcury);

void attoLNode_moveCursor(attoLineNode * restrict node, int32_t delta);

void attoLNode_destroy(attoLineNode * restrict node);

typedef struct attoFile
{
	const wchar_t * fileName;
	HANDLE hFile;
	bool canWrite;
	struct
	{
		attoLineNode * firstNode;
		attoLineNode * currentNode;
		attoLineNode * pcury;
		uint32_t curx;
	} data;
} attoFile;

bool attoFile_open(attoFile * restrict file, const wchar_t * restrict fileName, bool writemode);
void attoFile_close(attoFile * restrict file);
void attoFile_clearLines(attoFile * restrict file);
const wchar_t * attoFile_read(attoFile * restrict file);

enum attoFile_writeRes
{
	writeRes_nothingNew = -1,
	writeRes_openError  = -2,
	writeRes_writeError = -3,
	writeRes_memError   = -4
};

int attoFile_write(attoFile * restrict file);
void attoFile_setConTitle(attoFile * restrict file);

bool attoFile_addNormalCh(attoFile * restrict file, wchar_t ch);
bool attoFile_addSpecialCh(attoFile * restrict file, wchar_t ch);

bool attoFile_checkLineAt(const attoFile * restrict file, int32_t maxdelta, const wchar_t * string, uint32_t maxString);
bool attoFile_deleteForward(attoFile * restrict file);
bool attoFile_deleteBackward(attoFile * restrict file);
bool attoFile_addNewLine(attoFile * restrict file);
void attoFile_updateCury(attoFile * restrict file, uint32_t height);

void attoFile_destruct(attoFile * restrict file);


#endif
