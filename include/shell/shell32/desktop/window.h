#ifndef WINDOW_H
#define WINDOW_H

#include <via/stdio.h>

typedef struct HAND
{
	char sName[255];
};

typedef struct WND32_STATUS
{
	int x;
	int y;
	int w;
	int h;
};

typedef int WND32_COM;

typedef struct WNDCLASS
{
	struct HAND handle;
	struct WND32_STATUS status;
	uint8_t WND32_COL;
	void (*wndproc)();
};

void AllowSetForeground(uint8_t color);
void WND32_Paint(struct WNDCLASS wndClass, void (*proc)());
void PaintDesktop(struct WNDCLASS* wndClasses);

int GetWND32_Msg();

#endif