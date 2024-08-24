#include <via/stdio.h>

void list_dir(struct RIFS_F* a)
{
	printf("\nDirectory: / \n");

	for(int i = 0; i < 3; i++)
	{
		VDK_InterpretFile(a[i].metadata);
	}

	printf("> ");
}