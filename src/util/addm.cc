#include <stdlib.h>
#include <stdio.h>


void
addm ()
{
  int character;
  while ((character = getchar ()) != EOF)
  {
	if (character == 10)
	{
	  putchar (13);
	}
	putchar (character);
  }
}

int
main (int argc, char * argv[])
{
  if (argc == 1)
  {
	addm ();
  }
  else
  {
	for (int i = 1; i < argc; i++)
	{
	  char outname[1024];
	  sprintf (outname, "%s_temp", argv[i]);

	  freopen (argv[i], "r", stdin);
	  freopen (outname, "w", stdout);

	  addm ();

	  char command[1024];
	  sprintf (command, "mv %s %s", outname, argv[i]);
	  system (command);
	}
  }

  return 0;
}
