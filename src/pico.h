#ifndef PICO_H
#define PICO_H

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

#define PICO_LNODE_DEFAULT_FREE 10

typedef struct pico_LNode
{
	wchar_t * line;
	uint32_t lineEndx, curx, freeSpaceLen;
	struct pico_LNode * prevNode, * nextNode;
} pico_LNode;

pico_LNode * picoLNode_create(pico_LNode * curnode, pico_LNode * nextnode);
pico_LNode * picoLNode_createText(
	pico_LNode * curnode,
	pico_LNode * nextnode,
	const wchar_t * lineText,
	int32_t maxText
);
bool picoLNode_realloc(pico_LNode * restrict curnode);

bool picoLNode_merge(pico_LNode * restrict node);

void picoLNode_destroy(pico_LNode * restrict node);

typedef struct pico_File
{
	const wchar_t * fileName;
	HANDLE hFile;
	bool canWrite;
	struct
	{
		pico_LNode * firstNode;
		pico_LNode * currentNode;
		pico_LNode * cury;
		uint32_t curx;
	} data;
} pico_File;

bool picoFile_open(pico_File * restrict file, const wchar_t * restrict fileName);
void picoFile_clearLines(pico_File * restrict file);
const wchar_t * picoFile_read(pico_File * restrict file);
int picoFile_write(pico_File * restrict file);
void picoFile_setConTitle(pico_File * restrict file);

bool picoFile_addNormalCh(pico_File * restrict file, wchar_t ch);
bool picoFile_addSpecialCh(pico_File * restrict file, wchar_t ch);

bool picoFile_checkLineAt(const pico_File * restrict file, int32_t maxdelta, const wchar_t * string, uint32_t maxString);
bool picoFile_deleteForward(pico_File * restrict file);
bool picoFile_deleteBackward(pico_File * restrict file);
bool picoFile_addNewLine(pico_File * restrict file);

void picoFile_destruct(pico_File * restrict file);

typedef struct pico_DS
{
	HANDLE conIn, conOut;
	struct
	{
		HANDLE handle;
		wchar_t * mem;
		uint32_t w, h;
	} scrbuf;
	COORD cursorpos;
} pico_DS;

bool picoDS_init(pico_DS * restrict ds);
void picoDS_refresh(pico_DS * restrict ds);
void picoDS_refreshAll(pico_DS * restrict ds);
void picoDS_statusDraw(pico_DS * restrict ds, const wchar_t * message);
void picoDS_statusRefresh(pico_DS * restrict ds);

void picoDS_destruct(pico_DS * restrict ds);

void pico_exitHandler();

const wchar_t * pico_getFileName(const int argc, const wchar_t * const * const argv);
void pico_printHelp(const wchar_t * app);

enum picoE
{
	picoE_unknown,
	picoE_file,
	picoE_window,

	picoE_num_of_elems
};
void pico_printErr(enum picoE errCode);
bool pico_loop();
void pico_updateScrbuf();


uint32_t pico_convToUnicode(const char * utf8, int numBytes, wchar_t ** putf16);
uint32_t pico_convFromUnicode(const wchar_t * utf16, char ** putf8);
uint32_t pico_strnToLines(wchar_t * utf16, uint32_t chars, wchar_t *** lines);

#endif
