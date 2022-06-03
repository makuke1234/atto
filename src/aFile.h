#ifndef ATTO_FILE_H
#define ATTO_FILE_H

#include "aCommon.h"

#define ATTO_LNODE_DEFAULT_FREE 10

/*
	Example:
	L"This is text\0\0\0\0\0\0"
	              ^ - curx
				  <----------> - freeSpaceLen = 6
*/

typedef struct aLine
{
	wchar * line;
	usize lineEndx, curx, freeSpaceLen;

	struct aLine * prevNode, * nextNode;

} aLine_t;

/**
 * @brief Creates new line in-between current line and next line
 * 
 * @param curnode Pointer to current line node, can be NULL
 * @param nextnode Pointer to next line node, can be NULL
 * @return aLine_t* Pointer to newly created line node, NULL on failure
 */
aLine_t * aLine_create(aLine_t * restrict curnode, aLine_t * restrict nextnode);
/**
 * @brief Creates new line in-between current line and next line
 * 
 * @param curnode Pointer to current line node, can be NULL
 * @param nextnode Pointer to next line node, can be NULL
 * @param lineText Pointer to UTF-16 character array, contents of this will be
 * copied to the newly created line
 * @param maxText Maximum amount of characters to copy (not including null-terminator),
 * can be -1, if string is null-terminated
 * @return aLine_t* Pointer to newly created line node, NULL on failure
 */
aLine_t * aLine_createText(
	aLine_t * restrict curnode,
	aLine_t * restrict nextnode,
	const wchar * restrict lineText,
	isize maxText
);

/**
 * @brief Fetches text from given line node, copies it to wchar character array,
 * allocates memory only if *text is too small or tarrsz == NULL
 * 
 * @param self Pointer to line node to fetch text from
 * @param text Address of wchar pointer to character array, wchar pointer
 * itself can be initally NULL
 * @param tarrsz Size of receiving character array, can be NULL
 * @return true Success
 * @return false Failure
 */
bool aLine_getText(const aLine_t * restrict self, wchar ** restrict text, usize * restrict tarrsz);
/**
 * @brief Reallocates free space on given line node, guarantees
 * ATTO_LNODE_DEFAULT_FREE characters for space
 * 
 * @param self Pointer to line node
 * @return true Success
 * @return false Failure
 */
bool aLine_realloc(aLine_t * restrict self);

/**
 * @brief Merges current line node with next line node, adjusts current
 * y-position line node pointer if necessary
 * 
 * @param self Pointer to current line node
 * @param ppcury Address of pointer to current y-position line node
 * @return true Success
 * @return false Failure
 */
bool aLine_mergeNext(aLine_t * restrict self, aLine_t ** restrict ppcury);

/**
 * @brief Moves (internal) cursor on current line node, clamps movement
 * 
 * @param self Pointer to current line node
 * @param delta Amount of characters to move, positive values to move right,
 * negative values to move left
 */
void aLine_moveCursor(aLine_t * restrict self, isize delta);

/**
 * @brief Destroys line node, frees memory
 * 
 * @param self Pointer to line node
 */
void aLine_destroy(aLine_t * restrict self);

typedef enum eolSequence
{
	eolNOT  = 0x00,
	eolCR   = 0x01,
	eolLF   = 0x02,
	eolCRLF = eolCR | eolLF,

	eolDEF  = eolCRLF

} eolSequence_e, eolSeq_e;

typedef struct aFile
{
	const wchar * fileName;
	HANDLE hFile;
	bool canWrite;
	eolSeq_e eolSeq;

	struct
	{
		aLine_t * firstNode;
		aLine_t * currentNode;
		aLine_t * pcury;
		usize curx;
	} data;

} aFile_t;

/**
 * @brief Resets aFile_t structure memory layout, zeroes all members
 * 
 * @param self Pointer to aFile_t structure
 */
void aFile_reset(aFile_t * restrict self);
/**
 * @brief Opens new file with desired name and write access
 * 
 * @param self Pointer to aFile_t structure
 * @param selfName Desired file name, can be NULL; If is NULL, previously given file
 * name will be used
 * @param writemode Desires write access, true for write mode, false for read mode
 * @return true Success
 * @return false Failure
 */
bool aFile_open(aFile_t * restrict self, const wchar * restrict selfName, bool writemode);
/**
 * @brief Closes open file if possible
 * 
 * @param self Pointer to aFile_t structure
 */
void aFile_close(aFile_t * restrict self);
/**
 * @brief Clears (internal) lines in editor data structure
 * 
 * @param self Pointer to aFile_t structure
 */
void aFile_clearLines(aFile_t * restrict self);
/**
 * @brief Opens file with last given filename, reads bytes to an array, allocates
 * memory only if *bytes is too small
 * 
 * @param self Pointer to aFile_t structure
 * @param bytes Address of pointer to character array
 * @param bytesLen Address of array length in bytes
 * @return const wchar* Error message, NULL on success
 */
const wchar * aFile_readBytes(aFile_t * restrict self, char ** restrict bytes, usize * restrict bytesLen);
/**
 * @brief Opens file with last given filename, reads file contents to internal
 * structure, ready to be shown on screen
 * 
 * @param self Pointer to aFile_t structure
 * @return const wchar* Error message, NULL on success
 */
const wchar * aFile_read(aFile_t * restrict self);

typedef enum aFile_writeRes
{
	afwrNOTHING_NEW = -1,
	afwrOPEN_ERROR  = -2,
	afwrWRITE_ERROR = -3,
	afwrMEM_ERROR   = -4

} aFile_writeRes_e, aFile_wr_e, afwr_e;
/**
 * @brief Opens file with last given filename, reads file contents to temporary memory,
 * compares them with current internal file contents in memory. Only attempts to write to
 * file if anything has been changed
 * 
 * @param self Pointer to aFile_t structure
 * @return isize Negative values represent error code, positive values (0 inclusive)
 * represent number of bytes written to disc
 */
isize aFile_write(aFile_t * restrict self);
/**
 * @brief Set console title according to last given filename, also shows
 * editor name on the titlebar
 * 
 * @param self Pointer to aFile_t structure
 */
void aFile_setConTitle(const aFile_t * restrict self);

/**
 * @brief Inserts a normal character to current line
 * 
 * @param self Pointer to aFile_t structure
 * @param ch Character to insert
 * @return true Success
 * @return false Failure
 */
bool aFile_addNormalCh(aFile_t * restrict self, wchar ch);
/**
 * @brief Inserts a special character to current line
 * 
 * @param self Pointer to aFile_t structure
 * @param ch Character to insert
 * @return true Success
 * @return false Failure
 */
bool aFile_addSpecialCh(aFile_t * restrict self, wchar ch);

/**
 * @brief Checks current line contents for matching string
 * 
 * @param self Pointer to aFile_t structure
 * @param maxdelta Offset from current cursor position, value is clamped
 * @param string Pointer to character array to match with
 * @param maxString Absolute maximum number of characters to check, stops anyway on null-terminator
 * @return true Found a match
 * @return false Didn't find any match
 */
bool aFile_checkLineAt(const aFile_t * restrict self, isize maxdelta, const wchar * restrict string, usize maxString);
/**
 * @brief Deletes a character on current line going forward (right, cursor stationary),
 * merges with the next line (if possible), if cursor is already at the end of the line
 * 
 * @param self Pointer to aFile_t structure
 * @return true Success
 * @return false Failure
 */
bool aFile_deleteForward(aFile_t * restrict self);
/**
 * @brief Deletes a character on current line going backwards (left, cursor also move to the left),
 * merges with and move to the the previous line (if possible), if cursor is
 * already at the beginning of the line
 * 
 * @param self Pointer to aFile_t structure
 * @return true Success
 * @return false Failure
 */
bool aFile_deleteBackward(aFile_t * restrict self);
/**
 * @brief Adds a new line after current active line
 * 
 * @param self Pointer to aFile_t structure
 * @return true Success
 * @return false Failure
 */
bool aFile_addNewLine(aFile_t * restrict self);
/**
 * @brief Updates current viewpoint if necessary, shifts view vertically
 * 
 * @param self Pointer to aFile_t structure
 * @param height Editor window height
 */
void aFile_updateCury(aFile_t * restrict self, u32 height);

/**
 * @brief Destroys aFile_t structure
 * 
 * @param self Pointer to aFile_t structure
 */
void aFile_destroy(aFile_t * restrict self);


#endif
