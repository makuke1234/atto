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

void attoData_reset(attoData_t * restrict self);
bool attoData_init(attoData_t * restrict self);
void attoData_refresh(attoData_t * restrict self);
void attoData_refreshAll(attoData_t * restrict self);
void attoData_statusDraw(attoData_t * restrict self, const wchar_t * message);
void attoData_statusRefresh(attoData_t * restrict self);

void attoData_destruct(attoData_t * restrict self);

#endif
