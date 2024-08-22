#include <via/stdio.h>

// FIXME: Shitty system, i should have a list of created files instead.
void list_dir(struct RIFS_File* a)
{
	printf("\nDirectory: / \n");

	for(int i = 0; i < 2; i++)
	{
		VDK_InterpretFile(a[i]);
	}

	printf("> ");
}