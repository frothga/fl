#include <stdlib.h>
#include <stdio.h>


void
striputf8 ()
{
  int character;

  // Eat the endian indicator at start of file.  For now, just assume smalle endian.
  getchar ();
  getchar ();

  while ((character = getchar (), getchar ()) != EOF)
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
	striputf8 ();
  }
  else
  {
	for (int i = 1; i < argc; i++)
	{
	  char outname[1024];
	  sprintf (outname, "%s_temp", argv[i]);

	  freopen (argv[i], "r", stdin);
	  freopen (outname, "w", stdout);

	  striputf8 ();

	  char command[1024];
	  sprintf (command, "mv %s %s", outname, argv[i]);
	  system (command);
	}
  }

  return 0;
}
