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

typedef struct attoLineNode_t
{
	wchar_t * line;
	uint32_t lineEndx, curx, freeSpaceLen;
	struct attoLineNode_t * prevNode, * nextNode;
} attoLineNode_t;

attoLineNode_t * attoLine_create(attoLineNode_t * curnode, attoLineNode_t * nextnode);
attoLineNode_t * attoLine_createText(
	attoLineNode_t * restrict curnode,
	attoLineNode_t * restrict nextnode,
	const wchar_t * restrict lineText,
	int32_t maxText
);

bool attoLine_getText(const attoLineNode_t * restrict self, wchar_t ** restrict text, uint32_t * restrict tarrsz);

bool attoLine_realloc(attoLineNode_t * restrict self);

bool attoLine_mergeNext(attoLineNode_t * restrict self, attoLineNode_t ** restrict ppcury);

void attoLine_moveCursor(attoLineNode_t * restrict self, int32_t delta);

void attoLine_destroy(attoLineNode_t * restrict self);

typedef struct attoFile_t
{
	const wchar_t * fileName;
	HANDLE hFile;
	bool canWrite;
	struct
	{
		attoLineNode_t * firstNode;
		attoLineNode_t * currentNode;
		attoLineNode_t * pcury;
		uint32_t curx;
	} data;
} attoFile_t;

bool attoFile_open(attoFile_t * restrict self, const wchar_t * restrict selfName, bool writemode);
void attoFile_close(attoFile_t * restrict self);
void attoFile_clearLines(attoFile_t * restrict self);
const wchar_t * attoFile_read(attoFile_t * restrict self);

enum attoFile_writeRes
{
	writeRes_nothingNew = -1,
	writeRes_openError  = -2,
	writeRes_writeError = -3,
	writeRes_memError   = -4
};

int attoFile_write(attoFile_t * restrict self);
void attoFile_setConTitle(attoFile_t * restrict self);

bool attoFile_addNormalCh(attoFile_t * restrict self, wchar_t ch);
bool attoFile_addSpecialCh(attoFile_t * restrict self, wchar_t ch);

bool attoFile_checkLineAt(const attoFile_t * restrict self, int32_t maxdelta, const wchar_t * restrict string, uint32_t maxString);
bool attoFile_deleteForward(attoFile_t * restrict self);
bool attoFile_deleteBackward(attoFile_t * restrict self);
bool attoFile_addNewLine(attoFile_t * restrict self);
void attoFile_updateCury(attoFile_t * restrict self, uint32_t height);

void attoFile_destruct(attoFile_t * restrict self);


#endif
