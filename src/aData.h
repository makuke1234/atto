#ifndef ATTO_DATA_H
#define ATTO_DATA_H

#include "aCommon.h"
#include "aFile.h"


typedef struct aData
{
	HANDLE conIn, conOut;
	struct
	{
		HANDLE handle;
		wchar * mem;
		u32 w, h;
	} scrbuf;
	COORD cursorpos;

	aFile_t file;

} aData_t;

/**
 * @brief Resets internal memory fields, zeroes them
 * 
 * @param self Pointer to aData_t structure
 */
void aData_reset(aData_t * restrict self);
/**
 * @brief Initialises editor data strucutre, also sets atexit() handler function
 * 
 * @param self Pointer to aData_t structure
 * @return true Success
 * @return false Failure
 */
bool aData_init(aData_t * restrict self);
/**
 * @brief Refreshes the screen's editing part only
 * 
 * @param self Pointer to aData_t structure
 */
void aData_refresh(aData_t * restrict self);
/**
 * @brief Refreshes whole screen
 * 
 * @param self Pointer to aData_t structure
 */
void aData_refreshAll(aData_t * restrict self);
/**
 * @brief Draws a status bar message, refreshes statusbar
 * 
 * @param self Pointer to aData_t structure
 * @param message 
 */
void aData_statusDraw(aData_t * restrict self, const wchar * restrict message);
/**
 * @brief Refreshes status bar
 * 
 * @param self Pointer to aData_t structure
 */
void aData_statusRefresh(aData_t * restrict self);

/**
 * @brief Destroys editor's data structure, frees memory
 * 
 * @param self Pointer to aData_t structure
 */
void aData_destroy(aData_t * restrict self);

#endif
