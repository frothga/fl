#define _XOPEN_SOURCE
#include <unistd.h>
#include <time.h>
#include <stdio.h>


int
main (int argc, char * argv[])
{
  const char codes[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  int seed = time (NULL) % 4096;
  char salt[3];
  salt[0] = codes[seed & 0x3F];
  salt[1] = codes[seed >> 6];
  salt[2] = 0;

  printf ("%s\n", crypt (argv[1], salt));

  return 0;
}
