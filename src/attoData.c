#include "attoData.h"
#include "atto.h"

#include <stdlib.h>

bool attoDS_init(attoData_t * restrict self)
{
	self->conIn  = GetStdHandle(STD_INPUT_HANDLE);
	self->conOut = GetStdHandle(STD_OUTPUT_HANDLE);
	// Set exit handler
	atexit(&atto_exitHandler);

	// Get console current size
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(self->conOut, &csbi))
	{
		return false;
	}

	self->scrbuf.w = (uint32_t)(csbi.srWindow.Right  - csbi.srWindow.Left + 1);
	self->scrbuf.h = (uint32_t)(csbi.srWindow.Bottom - csbi.srWindow.Top  + 1);
	// Create screen buffer
	self->scrbuf.handle = CreateConsoleScreenBuffer(
		GENERIC_WRITE,
		0,
		NULL,
		CONSOLE_TEXTMODE_BUFFER,
		NULL
	);
	if (self->scrbuf.handle == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	self->scrbuf.mem = malloc((size_t)(self->scrbuf.w * self->scrbuf.h) * sizeof(wchar_t));
	if (self->scrbuf.mem == NULL)
	{
		return false;
	}

	for (uint32_t i = 0, sz = self->scrbuf.w * self->scrbuf.h; i < sz; ++i)
	{
		self->scrbuf.mem[i] = L' ';
	}
	if (!SetConsoleScreenBufferSize(self->scrbuf.handle, (COORD){ .X = (SHORT)self->scrbuf.w, .Y = (SHORT)self->scrbuf.h }))
	{
		return false;
	}
	if (!SetConsoleActiveScreenBuffer(self->scrbuf.handle))
	{
		return false;
	}

	return true;
}
void attoDS_refresh(attoData_t * restrict self)
{
	atto_updateScrbuf();
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		self->scrbuf.handle,
		self->scrbuf.mem,
		self->scrbuf.w * (self->scrbuf.h - 1),
		(COORD){ 0, 0 },
		&dwBytes
	);
}
void attoDS_refreshAll(attoData_t * restrict self)
{
	atto_updateScrbuf();
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		self->scrbuf.handle,
		self->scrbuf.mem,
		self->scrbuf.w * self->scrbuf.h,
		(COORD){ 0, 0 },
		&dwBytes
	);
}
void attoDS_statusDraw(attoData_t * restrict self, const wchar_t * message)
{
	size_t len = wcslen(message), effLen = (len > self->scrbuf.w) ? (size_t)self->scrbuf.w : len;
	wchar_t * restrict lastLine = self->scrbuf.mem + (self->scrbuf.h - 1) * self->scrbuf.w;
	memcpy(
		lastLine,
		message,
		sizeof(wchar_t) * effLen
	);
	for (size_t i = effLen; i < self->scrbuf.w; ++i)
	{
		lastLine[i] = L' ';
	}
	attoDS_statusRefresh(self);
}
void attoDS_statusRefresh(attoData_t * restrict self)
{
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		self->scrbuf.handle,
		self->scrbuf.mem + (self->scrbuf.h - 1) * self->scrbuf.w,
		self->scrbuf.w,
		(COORD){ .X = 0, .Y = (SHORT)(self->scrbuf.h - 1) },
		&dwBytes
	);
}

void attoDS_destruct(attoData_t * restrict self)
{
	if (self->scrbuf.mem)
	{
		free(self->scrbuf.mem);
		self->scrbuf.mem = NULL;
	}
	if (self->scrbuf.handle)
	{
		SetConsoleActiveScreenBuffer(self->conOut);
	}
}
