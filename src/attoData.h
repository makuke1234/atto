#ifndef ATTODATA_H
#define ATTODATA_H

#include "common.h"
#include "attoFile.h"


typedef struct attoData_t
{
	HANDLE conIn, conOut;
	struct
	{
		HANDLE handle;
		wchar_t * mem;
		uint32_t w, h;
	} scrbuf;
	COORD cursorpos;

	attoFile_t file;
} attoData_t;

/**
 * @brief Resets internal memory fields, zeroes them
 * 
 * @param self Pointer to attoData_t structure
 */
void attoData_reset(attoData_t * restrict self);
/**
 * @brief Initialises editor data strucutre, also sets atexit() handler function
 * 
 * @param self Pointer to attoData_t structure
 * @return true Success
 * @return false Failure
 */
bool attoData_init(attoData_t * restrict self);
/**
 * @brief Refreshes the screen's editing part only
 * 
 * @param self Pointer to attoData_t structure
 */
void attoData_refresh(attoData_t * restrict self);
/**
 * @brief Refreshes whole screen
 * 
 * @param self Pointer to attoData_t structure
 */
void attoData_refreshAll(attoData_t * restrict self);
/**
 * @brief Draws a status bar message, refreshes statusbar
 * 
 * @param self Pointer to attoData_t structure
 * @param message 
 */
void attoData_statusDraw(attoData_t * restrict self, const wchar_t * message);
/**
 * @brief Refreshes status bar
 * 
 * @param self Pointer to attoData_t structure
 */
void attoData_statusRefresh(attoData_t * restrict self);

/**
 * @brief Destroys editor's data structure, frees memory
 * 
 * @param self Pointer to attoData_t structure
 */
void attoData_destroy(attoData_t * restrict self);

#endif
