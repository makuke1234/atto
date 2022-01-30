#ifndef ATTODATA_H
#define ATTODATA_H

#include "common.h"

#include <stdbool.h>
#include <stdint.h>

#define MAX_STATUS 256

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
} attoData_t;

bool attoDS_init(attoData_t * restrict self);
void attoDS_refresh(attoData_t * restrict self);
void attoDS_refreshAll(attoData_t * restrict self);
void attoDS_statusDraw(attoData_t * restrict self, const wchar_t * message);
void attoDS_statusRefresh(attoData_t * restrict self);

void attoDS_destruct(attoData_t * restrict self);

#endif
