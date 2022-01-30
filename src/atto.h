#ifndef ATTO_H
#define ATTO_H

#include "common.h"
#include "attoFile.h"
#include "attoData.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


extern attoFile file;
extern attoData editor;


bool boolGet(uint8_t * arr, const size_t index);
void boolPut(uint8_t * arr, const size_t index, const bool value);

int32_t i32Min(int32_t a, int32_t b);
int32_t i32Min(int32_t a, int32_t b);
uint32_t u32Min(uint32_t a, uint32_t b);
uint32_t u32Max(uint32_t a, uint32_t b);


void atto_exitHandler();

const wchar_t * atto_getFileName(const int argc, const wchar_t * const * const argv);
void atto_printHelp(const wchar_t * app);

enum attoErr
{
	attoE_unknown,
	attoE_file,
	attoE_window,

	attoE_num_of_elems
};
void atto_printErr(enum attoErr errCode);
bool atto_loop();
void atto_updateScrbuf();


uint32_t atto_convToUnicode(const char * utf8, int numBytes, wchar_t ** putf16, uint32_t * sz);
uint32_t atto_convFromUnicode(const wchar_t * utf16, int numChars, char ** putf8, uint32_t * sz);
uint32_t atto_strnToLines(wchar_t * utf16, uint32_t chars, wchar_t *** lines);

#endif
