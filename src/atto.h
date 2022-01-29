#ifndef atto_H
#define atto_H

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define NOMINMAX

#ifndef UNICODE
	#define UNICODE
#endif
#ifndef _UNICODE
	#define _UNICODE
#endif

#include <windows.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_STATUS 256

bool boolGet(uint8_t * arr, const size_t index);
void boolPut(uint8_t * arr, const size_t index, const bool value);

int32_t i32Min(int32_t a, int32_t b);
int32_t i32Min(int32_t a, int32_t b);
uint32_t u32Min(uint32_t a, uint32_t b);
uint32_t u32Max(uint32_t a, uint32_t b);


/*
	Example:
	L"This is text\0\0\0\0\0\0"
	              ^ - curx
				  <----------> - freeSpaceLen = 6

*/

#define atto_LNODE_DEFAULT_FREE 10

typedef struct atto_LNode
{
	wchar_t * line;
	uint32_t lineEndx, curx, freeSpaceLen;
	struct atto_LNode * prevNode, * nextNode;
} atto_LNode;

atto_LNode * attoLNode_create(atto_LNode * curnode, atto_LNode * nextnode);
atto_LNode * attoLNode_createText(
	atto_LNode * curnode,
	atto_LNode * nextnode,
	const wchar_t * lineText,
	int32_t maxText
);

bool attoLNode_getText(const atto_LNode * node, wchar_t ** text, uint32_t * tarrsz);

bool attoLNode_realloc(atto_LNode * restrict curnode);

bool attoLNode_merge(atto_LNode * restrict node, atto_LNode ** restrict ppcury);

void attoLNode_moveCursor(atto_LNode * restrict node, int32_t delta);

void attoLNode_destroy(atto_LNode * restrict node);

typedef struct atto_File
{
	const wchar_t * fileName;
	HANDLE hFile;
	bool canWrite;
	struct
	{
		atto_LNode * firstNode;
		atto_LNode * currentNode;
		atto_LNode * pcury;
		uint32_t curx;
	} data;
} atto_File;

bool attoFile_open(atto_File * restrict file, const wchar_t * restrict fileName, bool writemode);
void attoFile_close(atto_File * restrict file);
void attoFile_clearLines(atto_File * restrict file);
const wchar_t * attoFile_read(atto_File * restrict file);

enum attoFile_writeRes
{
	writeRes_nothingNew = -1,
	writeRes_openError  = -2,
	writeRes_writeError = -3,
	writeRes_memError   = -4
};

int attoFile_write(atto_File * restrict file);
void attoFile_setConTitle(atto_File * restrict file);

bool attoFile_addNormalCh(atto_File * restrict file, wchar_t ch);
bool attoFile_addSpecialCh(atto_File * restrict file, wchar_t ch);

bool attoFile_checkLineAt(const atto_File * restrict file, int32_t maxdelta, const wchar_t * string, uint32_t maxString);
bool attoFile_deleteForward(atto_File * restrict file);
bool attoFile_deleteBackward(atto_File * restrict file);
bool attoFile_addNewLine(atto_File * restrict file);
void attoFile_updateCury(atto_File * restrict file, uint32_t height);

void attoFile_destruct(atto_File * restrict file);

typedef struct atto_DS
{
	HANDLE conIn, conOut;
	struct
	{
		HANDLE handle;
		wchar_t * mem;
		uint32_t w, h;
	} scrbuf;
	COORD cursorpos;
} atto_DS;

bool attoDS_init(atto_DS * restrict ds);
void attoDS_refresh(atto_DS * restrict ds);
void attoDS_refreshAll(atto_DS * restrict ds);
void attoDS_statusDraw(atto_DS * restrict ds, const wchar_t * message);
void attoDS_statusRefresh(atto_DS * restrict ds);

void attoDS_destruct(atto_DS * restrict ds);

void atto_exitHandler();

const wchar_t * atto_getFileName(const int argc, const wchar_t * const * const argv);
void atto_printHelp(const wchar_t * app);

enum attoE
{
	attoE_unknown,
	attoE_file,
	attoE_window,

	attoE_num_of_elems
};
void atto_printErr(enum attoE errCode);
bool atto_loop();
void atto_updateScrbuf();


uint32_t atto_convToUnicode(const char * utf8, int numBytes, wchar_t ** putf16, uint32_t * sz);
uint32_t atto_convFromUnicode(const wchar_t * utf16, int numChars, char ** putf8, uint32_t * sz);
uint32_t atto_strnToLines(wchar_t * utf16, uint32_t chars, wchar_t *** lines);

#endif
