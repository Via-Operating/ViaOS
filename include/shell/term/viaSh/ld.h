#include <via/stdio.h>

void list_dir(struct RIFS_File* a)
{
	printf("\nDirectory: / \n");

	for(int i = 0; i < 2; i++)
	{
		VDK_InterpretFile(a[i]);
	}

	printf("> ");
}
