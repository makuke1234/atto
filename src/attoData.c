#include "attoData.h"
#include "atto.h"

#include <stdlib.h>

bool attoDS_init(attoData * restrict ds)
{
	ds->conIn  = GetStdHandle(STD_INPUT_HANDLE);
	ds->conOut = GetStdHandle(STD_OUTPUT_HANDLE);
	// Set exit handler
	atexit(&atto_exitHandler);

	// Get console current size
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(ds->conOut, &csbi))
	{
		return false;
	}

	ds->scrbuf.w = (uint32_t)(csbi.srWindow.Right  - csbi.srWindow.Left + 1);
	ds->scrbuf.h = (uint32_t)(csbi.srWindow.Bottom - csbi.srWindow.Top  + 1);
	// Create screen buffer
	ds->scrbuf.handle = CreateConsoleScreenBuffer(
		GENERIC_WRITE,
		0,
		NULL,
		CONSOLE_TEXTMODE_BUFFER,
		NULL
	);
	if (ds->scrbuf.handle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	ds->scrbuf.mem = malloc((size_t)(ds->scrbuf.w * ds->scrbuf.h) * sizeof(wchar_t));
	if (ds->scrbuf.mem == NULL)
	{
		return false;
	}

	for (uint32_t i = 0, sz = ds->scrbuf.w * ds->scrbuf.h; i < sz; ++i)
	{
		ds->scrbuf.mem[i] = L' ';
	}
	if (!SetConsoleScreenBufferSize(ds->scrbuf.handle, (COORD){ .X = (SHORT)ds->scrbuf.w, .Y = (SHORT)ds->scrbuf.h }))
	{
		return false;
	}
	if (!SetConsoleActiveScreenBuffer(ds->scrbuf.handle))
	{
		return false;
	}

	return true;
}
void attoDS_refresh(attoData * restrict ds)
{
	atto_updateScrbuf();
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		ds->scrbuf.handle,
		ds->scrbuf.mem,
		ds->scrbuf.w * (ds->scrbuf.h - 1),
		(COORD){ 0, 0 },
		&dwBytes
	);
}
void attoDS_refreshAll(attoData * restrict ds)
{
	atto_updateScrbuf();
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		ds->scrbuf.handle,
		ds->scrbuf.mem,
		ds->scrbuf.w * ds->scrbuf.h,
		(COORD){ 0, 0 },
		&dwBytes
	);
}
void attoDS_statusDraw(attoData * restrict ds, const wchar_t * message)
{
	size_t len = wcslen(message), effLen = (len > ds->scrbuf.w) ? (size_t)ds->scrbuf.w : len;
	wchar_t * restrict lastLine = ds->scrbuf.mem + (ds->scrbuf.h - 1) * ds->scrbuf.w;
	memcpy(
		lastLine,
		message,
		sizeof(wchar_t) * effLen
	);
	for (size_t i = effLen; i < ds->scrbuf.w; ++i)
	{
		lastLine[i] = L' ';
	}
	attoDS_statusRefresh(ds);
}
void attoDS_statusRefresh(attoData * restrict ds)
{
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		ds->scrbuf.handle,
		ds->scrbuf.mem + (ds->scrbuf.h - 1) * ds->scrbuf.w,
		ds->scrbuf.w,
		(COORD){ .X = 0, .Y = (SHORT)(ds->scrbuf.h - 1) },
		&dwBytes
	);
}

void attoDS_destruct(attoData * restrict ds)
{
	if (ds->scrbuf.mem)
	{
		free(ds->scrbuf.mem);
		ds->scrbuf.mem = NULL;
	}
	if (ds->scrbuf.handle)
	{
		SetConsoleActiveScreenBuffer(ds->conOut);
	}
}