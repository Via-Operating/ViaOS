#include <via/stdio.h>

void help_menu()
{
	printf("\nWelcome to ViaOS!\n");
	printf("\nhelp - Print out functionalities/commands existing of the operating system\n");
	printf("hi - Print out a sweet hello!\n");
	printf("ld - List Directory, print out files of current dir.\n");
	printf("whoami - List current user.\n");
	printf("cd - Change current Directory.\n");
	printf("info - Gives basic info on the user and shell.\n");
	printf("shutdown - Safely power down all processes to power off your computer safely.\n");

	printf("> ");
}