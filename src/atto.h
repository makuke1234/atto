#ifndef ATTO_H
#define ATTO_H

#include "common.h"
#include "attoFile.h"
#include "attoData.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_STATUS 256

extern attoFile_t file;
extern attoData_t editor;


bool boolGet(uint8_t * restrict arr, size_t index);
void boolPut(uint8_t * restrict arr, size_t index, bool value);

int32_t i32Min(int32_t a, int32_t b);
int32_t i32Max(int32_t a, int32_t b);
uint32_t u32Min(uint32_t a, uint32_t b);
uint32_t u32Max(uint32_t a, uint32_t b);


void atto_exitHandler(void);

const wchar_t * atto_getFileName(int argc, const wchar_t * const * const argv);
void atto_printHelp(const wchar_t * restrict app);

enum attoErr
{
	attoE_unknown,
	attoE_file,
	attoE_window,

	attoE_num_of_elems
};
void atto_printErr(enum attoErr errCode);
bool atto_loop(void);
void atto_updateScrbuf(void);


uint32_t atto_convToUnicode(const char * restrict utf8, int numBytes, wchar_t ** restrict putf16, uint32_t * restrict sz);
uint32_t atto_convFromUnicode(const wchar_t * restrict utf16, int numChars, char ** restrict putf8, uint32_t * restrict sz);
uint32_t atto_strnToLines(wchar_t * restrict utf16, uint32_t chars, wchar_t *** restrict lines);
uint32_t atto_tabsToSpaces(wchar_t ** restrict str, uint32_t * restrict len);

#endif
