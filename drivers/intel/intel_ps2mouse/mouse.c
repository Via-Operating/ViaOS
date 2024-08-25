#include "mouse.h"

// Initialize global variables for the mouse position and mouse status
int g_mouse_x_pos = 0, g_mouse_y_pos = 0;
VDK_MOUSE_STATUS g_status;

// Simple getters (JAVA FLASHBACKS)
int mouse_getx() 
{
    return g_mouse_x_pos;
}

int mouse_gety() 
{
    return g_mouse_y_pos;
}

void mouse_wait(enum BOOL_T type) 
{
    uint32_t time_out = 100000;
    if (type == FALSE) {
        // suspend until status is 1
        while (time_out--) {
            if ((ioport_in(PS2_CMD_PORT) & 1) == 1) {
                return;
            }
        }
        return;
    } else {
        while (time_out--) {
            if ((ioport_in(PS2_CMD_PORT) & 2) == 0) {
                return;
            }
        }
    }
}

void mouse_write(uint8_t data) 
{
    // sending write command
    mouse_wait(TRUE);
    ioport_out(PS2_CMD_PORT, 0xD4);
    mouse_wait(TRUE);
    // finally write data to port
    ioport_out(MOUSE_DATA_PORT, data);
}

uint8_t mouse_read() 
{
    mouse_wait(FALSE);
    return ioport_in(MOUSE_DATA_PORT);
}

void get_mouse_status(char status_byte, VDK_MOUSE_STATUS *status) 
{
    memset(status, 0, sizeof(VDK_MOUSE_STATUS));
    if (status_byte & 0x01)
        status->left_button = 1;
    if (status_byte & 0x02)
        status->right_button = 1;
    if (status_byte & 0x04)
        status->middle_button = 1;
    if (status_byte & 0x08)
        status->always_1 = 1;
    if (status_byte & 0x10)
        status->x_sign = 1;
    if (status_byte & 0x20)
        status->y_sign = 1;
    if (status_byte & 0x40)
        status->x_overflow = 1;
    if (status_byte & 0x80)
        status->y_overflow = 1;
}

void print_mouse_info() 
{
    reset_text_position();

    if (g_status.left_button) {
        bitmap_draw_string("Left button clicked", BLACK);
    }
    if (g_status.right_button) {
        bitmap_draw_string("Right button clicked", BLACK);
    }
    if (g_status.middle_button) {
        bitmap_draw_string("Middle button clicked", BLACK);
    }
}

void set_mouse_rate(uint8_t rate) 
{
    uint8_t status;

    ioport_out(MOUSE_DATA_PORT, MOUSE_CMD_SAMPLE_RATE);
    status = mouse_read();
    if(status != MOUSE_ACKNOWLEDGE) {
        //printf("error: failed to send mouse sample rate command\n");
        return;
    }
    ioport_out(MOUSE_DATA_PORT, rate);
    status = mouse_read();
    if(status != MOUSE_ACKNOWLEDGE) {
        //printf("error: failed to send mouse sample rate data\n");
        return;
    }
}

void mouse_init() 
{
    uint8_t status;

    g_mouse_x_pos = 5;
    g_mouse_y_pos = 2;

    //printf("initializing mouse...\n");

    // enable mouse device
    mouse_wait(TRUE);
    ioport_out(PS2_CMD_PORT, 0xA8);

    // print mouse id
    ioport_out(MOUSE_DATA_PORT, MOUSE_CMD_MOUSE_ID);
    status = mouse_read();
    //printf("mouse id: 0x%x\n", status);

    set_mouse_rate(10);

    //outportb(MOUSE_DATA_PORT, MOUSE_CMD_RESOLUTION);
    //outportb(MOUSE_DATA_PORT, 0);

    // enable the interrupt
    mouse_wait(TRUE);
    ioport_out(PS2_CMD_PORT, 0x20);
    mouse_wait(FALSE);
    // get and set second bit
    status = (ioport_in(MOUSE_DATA_PORT) | 2);
    // write status to port
    mouse_wait(TRUE);
    ioport_out(PS2_CMD_PORT, MOUSE_DATA_PORT);
    mouse_wait(TRUE);
    ioport_out(MOUSE_DATA_PORT, status);

    // set mouse to use default settings
    mouse_write(MOUSE_CMD_SET_DEFAULTS);
    status = mouse_read();
    if(status != MOUSE_ACKNOWLEDGE) {
        //printf("error: failed to set default mouse settings\n");
        return;
    }

    // enable packet streaming to receive
    mouse_write(MOUSE_CMD_ENABLE_PACKET_STREAMING);
    status = mouse_read();
    if(status != MOUSE_ACKNOWLEDGE) {
        //printf("error: failed to enable mouse packet streaming\n");
        return;
    }

    // set mouse handler
    //isr_register_interrupt_handler(IRQ_BASE + 12, mouse_handler);
}