#ifndef ATTODATA_H
#define ATTODATA_H

#include "common.h"

#include <stdbool.h>
#include <stdint.h>

#define MAX_STATUS 256

typedef struct attoData
{
	HANDLE conIn, conOut;
	struct
	{
		HANDLE handle;
		wchar_t * mem;
		uint32_t w, h;
	} scrbuf;
	COORD cursorpos;
} attoData;

bool attoDS_init(attoData * restrict ds);
void attoDS_refresh(attoData * restrict ds);
void attoDS_refreshAll(attoData * restrict ds);
void attoDS_statusDraw(attoData * restrict ds, const wchar_t * message);
void attoDS_statusRefresh(attoData * restrict ds);

void attoDS_destruct(attoData * restrict ds);

#endif
