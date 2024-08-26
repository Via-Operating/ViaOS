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
};

typedef struct WNDCLASS
{
	struct HAND handle;
	struct WND32_STATUS status;
	void (*wndproc)();
};

//void AllowSetForeground(enum VGA_COLOR_TYPE e);
void WND32_Paint(struct WNDCLASS wndClass, void (*proc)());
void PaintDesktop(struct WNDCLASS* wndClasses);

#endif