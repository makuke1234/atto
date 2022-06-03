#ifndef ATTO_H
#define ATTO_H

#include "aCommon.h"
#include "aData.h"

#define MAX_STATUS 256


i32 min_i32(i32 a, i32 b);
i32 max_i32(i32 a, i32 b);
u32 min_u32(u32 a, u32 b);
u32 max_u32(u32 a, u32 b);

usize min_usize(usize a, usize b);
usize max_usize(usize a, usize b);

/**
 * @brief Set exit handler data argument pointer
 * 
 * @param pdata Pointer to aData_t structure
 */
void atto_exitHandlerSetVars(aData_t * restrict pdata);
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
 * @return const wchar* File name argument
 */
const wchar * atto_getFileName(int argc, const wchar * const * const restrict argv);
void atto_printHelp(const wchar * restrict app);

typedef enum aErr
{
	aerrUNKNOWN,
	aerrFILE,
	aerrWINDOW,

	aerrNUM_OF_ELEMS

} aErr_e;
/**
 * @brief Prints error message to stderr, adds newline character
 * 
 * @param errCode Error code
 */
void atto_printErr(aErr_e errCode);
/**
 * @brief Performs text editor loop tasks
 * 
 * @param pdata Pointer to aData_t structure
 * @return true Normal operation
 * @return false Application is ready to quit
 */
bool atto_loop(aData_t * restrict pdata);
/**
 * @brief Update screen buffer
 * 
 * @param peditor Pointer to aData_t structure
 */
void atto_updateScrbuf(aData_t * restrict peditor);

/**
 * @brief Convert UTF-8 string to UTF-16 string, allocates memory only if
 * *putf16 is too small or sz == NULL
 * 
 * @param utf8 Pointer to UTF-8 character array
 * @param numBytes Maximum number of bytes to convert (including null-terminator)
 * @param putf16 Address of wchar pointer, the pointer itself can be initally NULL
 * @param sz Address of UTF-16 array size, can be NULL
 * @return u32 Number of characters converted
 */
u32 atto_toutf16(const char * restrict utf8, int numBytes, wchar ** restrict putf16, usize * restrict sz);
/**
 * @brief Convert UTF-16 string to UTF-8 string, allocates memory only if
 * *putf8 is too small or sz == NULL
 * 
 * @param utf16 Pointer to UTF-16 character array
 * @param numChars Maximum number of bytes to convert (including null-terminator)
 * @param putf8 Address of char pointer, the pointer itself can be initally NULL
 * @param sz Address of UTF-8 array size, can be NULL
 * @return u32 Number of characters converted
 */
u32 atto_toutf8(const wchar * restrict utf16, int numChars, char ** restrict putf8, usize * restrict sz);
/**
 * @brief Convert UTF-16 string to lines array, modifies original string. After
 * creation the double-pointer's "lines" can be safely freed with a single free.
 * It's just an array of pointer pointing to parts of the original string
 * 
 * @param utf16 Pointer to UTF-16 character array
 * @param chars Maximum number of bytes to scan
 * @param lines Address of wchar double-pointer, double-pointer hosts wchar
 * pointer array, where each pointer is a character array representing line in the text file.
 * Initial value of double-pointer is irrelevant
 * @param eolSeq Address of eolSequence enumerator, receives the EOL format used
 * @return usize Number of lines found
 */
usize atto_strnToLines(wchar * restrict utf16, usize chars, wchar *** restrict lines, eolSeq_e * restrict eolSeq);
/**
 * @brief Converts all tabs in string to spaces, modifies original string
 * 
 * @param str Address of a pointer to UTF-16 character array
 * @param len Address of string length (including null-terminator), can be NULL
 * @return usize New length of converted string (including null-terminator), same as *len, if len != NULL
 */
usize atto_tabsToSpaces(wchar ** restrict str, usize * restrict len);

#endif
