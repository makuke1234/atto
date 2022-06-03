#include "aData.h"
#include "atto.h"


void aData_reset(aData_t * restrict self)
{
	*self = (aData_t){
		.conIn  = INVALID_HANDLE_VALUE,
		.conOut = INVALID_HANDLE_VALUE,
		.scrbuf = {
			.handle = INVALID_HANDLE_VALUE,
			.mem    = NULL,
			.w      = 0,
			.h      = 0
		},
		.cursorpos = { 0, 0 }
	};
	aFile_reset(&self->file);
}
bool aData_init(aData_t * restrict self)
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

	self->scrbuf.w = (u32)(csbi.srWindow.Right  - csbi.srWindow.Left + 1);
	self->scrbuf.h = (u32)(csbi.srWindow.Bottom - csbi.srWindow.Top  + 1);
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

	self->scrbuf.mem = malloc((usize)self->scrbuf.w * (usize)self->scrbuf.h * sizeof(wchar));
	if (self->scrbuf.mem == NULL)
	{
		return false;
	}

	for (usize i = 0, sz = (usize)self->scrbuf.w * (usize)self->scrbuf.h; i < sz; ++i)
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
void aData_refresh(aData_t * restrict self)
{
	atto_updateScrbuf(self);
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		self->scrbuf.handle,
		self->scrbuf.mem,
		(DWORD)((usize)self->scrbuf.w * (usize)(self->scrbuf.h - 1)),
		(COORD){ 0, 0 },
		&dwBytes
	);
}
void aData_refreshAll(aData_t * restrict self)
{
	atto_updateScrbuf(self);
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		self->scrbuf.handle,
		self->scrbuf.mem,
		(DWORD)((usize)self->scrbuf.w * (usize)self->scrbuf.h),
		(COORD){ 0, 0 },
		&dwBytes
	);
}
void aData_statusDraw(aData_t * restrict self, const wchar * restrict message)
{
	const u32 effLen = (u32)min_usize(wcslen(message), (usize)self->scrbuf.w);
	wchar * restrict lastLine = self->scrbuf.mem + (usize)(self->scrbuf.h - 1) * (usize)self->scrbuf.w;
	memcpy(
		lastLine,
		message,
		sizeof(wchar) * (usize)effLen
	);
	for (u32 i = effLen; i < self->scrbuf.w; ++i)
	{
		lastLine[i] = L' ';
	}
	aData_statusRefresh(self);
}
void aData_statusRefresh(aData_t * restrict self)
{
	DWORD dwBytes;
	WriteConsoleOutputCharacterW(
		self->scrbuf.handle,
		self->scrbuf.mem + (usize)(self->scrbuf.h - 1) * (usize)self->scrbuf.w,
		(DWORD)self->scrbuf.w,
		(COORD){ .X = 0, .Y = (SHORT)(self->scrbuf.h - 1) },
		&dwBytes
	);
}

void aData_destroy(aData_t * restrict self)
{
	if (self->scrbuf.mem != NULL)
	{
		free(self->scrbuf.mem);
		self->scrbuf.mem = NULL;
	}
	if (self->scrbuf.handle != INVALID_HANDLE_VALUE)
	{
		SetConsoleActiveScreenBuffer(self->conOut);
	}
	aFile_destroy(&self->file);
}
