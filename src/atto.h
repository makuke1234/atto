#ifndef ATTO_H
#define ATTO_H

#include "common.h"
#include "attoData.h"

#define MAX_STATUS 256

/**
 * @brief Get value from compact boolean array
 * 
 * @param arr Pointer to array
 * @param index Item index
 * @return Item value
 */
bool boolGet(uint8_t * restrict arr, size_t index);
/**
 * @brief Put value to compact boolean array
 * 
 * @param arr Pointer to array
 * @param index Item index
 * @param value Item value
 */
void boolPut(uint8_t * restrict arr, size_t index, bool value);

int32_t i32Min(int32_t a, int32_t b);
int32_t i32Max(int32_t a, int32_t b);
uint32_t u32Min(uint32_t a, uint32_t b);
uint32_t u32Max(uint32_t a, uint32_t b);

/**
 * @brief Set exit handler data argument pointer
 * 
 * @param pdata Pointer to attoData_t structure
 */
void atto_exitHandlerSetVars(attoData_t * pdata);
/**
 * @brief Specialised exit handler, executed after returning from main
 * 
 */
void atto_exitHandler(void);

/**
 * @brief Gets file name from argument vector
 * 
 * @param argc Argument vector count
 * @param argv Wide-stringed argument vector
 * @return const wchar_t* File name argument
 */
const wchar_t * atto_getFileName(int argc, const wchar_t * const * const argv);
void atto_printHelp(const wchar_t * restrict app);

enum attoErr
{
	attoE_unknown,
	attoE_file,
	attoE_window,

	attoE_num_of_elems
};
/**
 * @brief Prints error message
 * 
 * @param errCode Error code
 */
void atto_printErr(enum attoErr errCode);
/**
 * @brief Performs text editor loop tasks
 * 
 * @param pdata Pointer to attoData_t structure
 * @return true Normal operation
 * @return false Application is ready to quit
 */
bool atto_loop(attoData_t * restrict pdata);
/**
 * @brief Update screen buffer
 * 
 * @param peditor Pointer to attoData_t structure
 */
void atto_updateScrbuf(attoData_t * restrict peditor);

/**
 * @brief Convert UTF-8 string to UTF-16 string, allocates memory only if
 * *putf16 is too small or sz == NULL
 * 
 * @param utf8 Pointer to UTF-8 character array
 * @param numBytes Maximum number of bytes to convert (including null-terminator)
 * @param putf16 Address of wchar_t pointer, the pointer itself can be initally NULL
 * @param sz Address of UTF-16 array size, can be NULL
 * @return uint32_t Number of characters converted
 */
uint32_t atto_convToUnicode(const char * restrict utf8, int numBytes, wchar_t ** restrict putf16, uint32_t * restrict sz);
/**
 * @brief Convert UTF-16 string to UTF-8 string, allocates memory only if
 * *putf8 is too small or sz == NULL
 * 
 * @param utf16 Pointer to UTF-16 character array
 * @param numChars Maximum number of bytes to convert (including null-terminator)
 * @param putf8 Address of char pointer, the pointer itself can be initally NULL
 * @param sz Address of UTF-8 array size, can be NULL
 * @return uint32_t Number of characters converted
 */
uint32_t atto_convFromUnicode(const wchar_t * restrict utf16, int numChars, char ** restrict putf8, uint32_t * restrict sz);
/**
 * @brief Convert UTF-16 string to lines array, modifies original string. After
 * creation the double-pointer's "lines" can be safely freed with a single free.
 * It's just an array of pointer pointing to parts of the original string
 * 
 * @param utf16 Pointer to UTF-16 character array
 * @param chars Maximum number of bytes to scan
 * @param lines Address of wchar_t double-pointer, double-pointer hosts wchar_t
 * pointer array, where each pointer is a character array representing line in the text file.
 * Initial value of double-pointer is irrelevant
 * @return uint32_t Number of lines found
 */
uint32_t atto_strnToLines(wchar_t * restrict utf16, uint32_t chars, wchar_t *** restrict lines);
/**
 * @brief Converts all tabs in string to spaces, modifies original string
 * 
 * @param str Address of a pointer to UTF-16 character array
 * @param len Address of string length (including null-terminator), can be NULL
 * @return uint32_t New length of converted string (including null-terminator), same as *len, if len != NULL
 */
uint32_t atto_tabsToSpaces(wchar_t ** restrict str, uint32_t * restrict len);

#endif
