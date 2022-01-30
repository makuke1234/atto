#ifndef ATTOFILE_H
#define ATTOFILE_H

#include "common.h"

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

/**
 * @brief Creates new line in-between current line and next line
 * 
 * @param curnode Pointer to current line node, can be NULL
 * @param nextnode Pointer to next line node, can be NULL
 * @return attoLineNode_t* Pointer to newly created line node, NULL on failure
 */
attoLineNode_t * attoLine_create(attoLineNode_t * restrict curnode, attoLineNode_t * restrict nextnode);
/**
 * @brief Creates new line in-between current line and next line
 * 
 * @param curnode Pointer to current line node, can be NULL
 * @param nextnode Pointer to next line node, can be NULL
 * @param lineText Pointer to UTF-16 character array, contents of this will be
 * copied to the newly created line
 * @param maxText Maximum amount of characters to copy (not including null-terminator),
 * can be -1, if string is null-terminated
 * @return attoLineNode_t* Pointer to newly created line node, NULL on failure
 */
attoLineNode_t * attoLine_createText(
	attoLineNode_t * restrict curnode,
	attoLineNode_t * restrict nextnode,
	const wchar_t * restrict lineText,
	int32_t maxText
);

/**
 * @brief Fetches text from given line node, copies it to wchar_t character array,
 * allocates memory only if *text is too small or tarrsz == NULL
 * 
 * @param self Pointer to line node to fetch text from
 * @param text Address of wchar_t pointer to character array, wchar_t pointer
 * itself can be initally NULL
 * @param tarrsz Size of receiving character array, can be NULL
 * @return true Success
 * @return false Failure
 */
bool attoLine_getText(const attoLineNode_t * restrict self, wchar_t ** restrict text, uint32_t * restrict tarrsz);
/**
 * @brief Reallocates free space on given line node, guarantees
 * ATTO_LNODE_DEFAULT_FREE characters for space
 * 
 * @param self Pointer to line node
 * @return true Success
 * @return false Failure
 */
bool attoLine_realloc(attoLineNode_t * restrict self);

/**
 * @brief Merges current line node with next line node, adjusts current
 * y-position line node pointer if necessary
 * 
 * @param self Pointer to current line node
 * @param ppcury Address of pointer to current y-position line node
 * @return true Success
 * @return false Failure
 */
bool attoLine_mergeNext(attoLineNode_t * restrict self, attoLineNode_t ** restrict ppcury);

/**
 * @brief Moves (internal) cursor on current line node, clamps movement
 * 
 * @param self Pointer to current line node
 * @param delta Amount of characters to move, positive values to move right,
 * negative values to move left
 */
void attoLine_moveCursor(attoLineNode_t * restrict self, int32_t delta);

/**
 * @brief Destroys line node, frees memory
 * 
 * @param self Pointer to line node
 */
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

/**
 * @brief Resets attoFile_t structure memory layout, zeroes all members
 * 
 * @param self Pointer to attoFile_t structure
 */
void attoFile_reset(attoFile_t * restrict self);
/**
 * @brief Opens new file with desired name and write access
 * 
 * @param self Pointer to attoFile_t structure
 * @param selfName Desired file name, can be NULL; If is NULL, previously given file
 * name will be used
 * @param writemode Desires write access, true for write mode, false for read mode
 * @return true Success
 * @return false Failure
 */
bool attoFile_open(attoFile_t * restrict self, const wchar_t * restrict selfName, bool writemode);
/**
 * @brief Closes open file if possible
 * 
 * @param self Pointer to attoFile_t structure
 */
void attoFile_close(attoFile_t * restrict self);
/**
 * @brief Clears (internal) lines in editor data structure
 * 
 * @param self Pointer to attoFile_t structure
 */
void attoFile_clearLines(attoFile_t * restrict self);
/**
 * @brief Opens file with last given filename, reads bytes to an array, allocates
 * memory only if *bytes is too small
 * 
 * @param self Pointer to attoFile_t structure
 * @param bytes Address of pointer to character array
 * @param bytesLen Address of array length in bytes
 * @return const wchar_t* Error message, NULL on success
 */
const wchar_t * attoFile_readBytes(attoFile_t * restrict self, char ** bytes, uint32_t * bytesLen);
/**
 * @brief Opens file with last given filename, reads file contents to internal
 * structure, ready to be shown on screen
 * 
 * @param self Pointer to attoFile_t structure
 * @return const wchar_t* Error message, NULL on success
 */
const wchar_t * attoFile_read(attoFile_t * restrict self);

enum attoFile_writeRes
{
	writeRes_nothingNew = -1,
	writeRes_openError  = -2,
	writeRes_writeError = -3,
	writeRes_memError   = -4
};
/**
 * @brief Opens file with last given filename, reads file contents to temporary memory,
 * compares them with current internal file contents in memory. Only attempts to write to
 * file if anything has been changed
 * 
 * @param self Pointer to attoFile_t structure
 * @return int Negative values represent error code, positive values (0 inclusive)
 * represent number of bytes written to disc
 */
int attoFile_write(attoFile_t * restrict self);
/**
 * @brief Set console title according to last given filename, also shows
 * editor name on the titlebar
 * 
 * @param self Pointer to attoFile_t structure
 */
void attoFile_setConTitle(const attoFile_t * restrict self);

/**
 * @brief Inserts a normal character to current line
 * 
 * @param self Pointer to attoFile_t structure
 * @param ch Character to insert
 * @return true Success
 * @return false Failure
 */
bool attoFile_addNormalCh(attoFile_t * restrict self, wchar_t ch);
/**
 * @brief Inserts a special character to current line
 * 
 * @param self Pointer to attoFile_t structure
 * @param ch Character to insert
 * @return true Success
 * @return false Failure
 */
bool attoFile_addSpecialCh(attoFile_t * restrict self, wchar_t ch);

/**
 * @brief Checks current line contents for matching string
 * 
 * @param self Pointer to attoFile_t structure
 * @param maxdelta Offset from current cursor position, value is clamped
 * @param string Pointer to character array to match with
 * @param maxString Absolute maximum number of characters to check, stops anyway on null-terminator
 * @return true Found a match
 * @return false Didn't find any match
 */
bool attoFile_checkLineAt(const attoFile_t * restrict self, int32_t maxdelta, const wchar_t * restrict string, uint32_t maxString);
/**
 * @brief Deletes a character on current line going forward (right, cursor stationary),
 * merges with the next line (if possible), if cursor is already at the end of the line
 * 
 * @param self Pointer to attoFile_t structure
 * @return true Success
 * @return false Failure
 */
bool attoFile_deleteForward(attoFile_t * restrict self);
/**
 * @brief Deletes a character on current line going backwards (left, cursor also move to the left),
 * merges with and move to the the previous line (if possible), if cursor is
 * already at the beginning of the line
 * 
 * @param self Pointer to attoFile_t structure
 * @return true Success
 * @return false Failure
 */
bool attoFile_deleteBackward(attoFile_t * restrict self);
/**
 * @brief Adds a new line after current active line
 * 
 * @param self Pointer to attoFile_t structure
 * @return true Success
 * @return false Failure
 */
bool attoFile_addNewLine(attoFile_t * restrict self);
/**
 * @brief Updates current viewpoint if necessary, shifts view vertically
 * 
 * @param self Pointer to attoFile_t structure
 * @param height Editor window height
 */
void attoFile_updateCury(attoFile_t * restrict self, uint32_t height);

/**
 * @brief Destroys attoFile_t structure
 * 
 * @param self Pointer to attoFile_t structure
 */
void attoFile_destroy(attoFile_t * restrict self);


#endif
