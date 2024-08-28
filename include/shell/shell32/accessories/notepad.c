#include "notepad.h"
#include "../../../../drivers/independent/graphics/vga/vga.h"
#include "../../../../drivers/independent/disk/ata/IDE.h"

// Create a handle, a status and a WNDCLASS.
struct HAND hwnd;
struct WND32_STATUS stat;
struct WNDCLASS notepad;

void NP_Open_File(uint32_t liba, char *buffer, size_t buffer_size)
{
	const int DRIVE = ata_get_drive_by_model("QEMU HARDDISK"); // qemu on top
    const uint32_t LBA = 0;
    const uint8_t NO_OF_SECTORS = 1;
    char buf[ATA_SECTOR_SIZE] = {0};

	memset(buffer, 0, sizeof(buffer));
    ide_read_sectors(DRIVE, NO_OF_SECTORS, LBA + liba, (uint32_t)buffer);
}

void NPproc()
{
    char buf[512];

    NP_Open_File(6, buf, 512);

	set_text_position(100, 50);
    bitmap_draw_stringS(" Desktop.CFG", RED, TRUE, 76);
    set_text_position(100, 92);
    bitmap_draw_stringS(buf, BLACK, TRUE, 92);
}

void NPinit()
{
	// Create a Via32 API Window.

	stat.x = 100;
	stat.y = 50;
	stat.w = 200;
	stat.h = 120;

	strcpy(hwnd.sName, " File Previewer");

	notepad.handle = hwnd;
	notepad.status = stat;

	// Set window foreground
    AllowSetForeground(WHITE);
}

void NPdraw()
{
	WND32_Paint(notepad, NPproc);
}