#include <stdlib.h>
#include <stdio.h>


void
stripm ()
{
  int character;
  while ((character = getchar ()) != EOF)
  {
	if (character != 13)
	{
	  putchar (character);
	}
  }
}

int
main (int argc, char * argv[])
{
  if (argc == 1)
  {
	stripm ();
  }
  else
  {
	for (int i = 1; i < argc; i++)
	{
	  char outname[1024];
	  sprintf (outname, "%s_temp", argv[i]);

	  freopen (argv[i], "r", stdin);
	  freopen (outname, "w", stdout);

	  stripm ();

	  char command[1024];
	  sprintf (command, "mv %s %s", outname, argv[i]);
	  system (command);
	}
  }

  return 0;
}
