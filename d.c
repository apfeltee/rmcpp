
#include <stdio.h>

/*
  1 "C:/cygwin/tmp/tmpxft_000042e8_00000000-7_a_dlink.fatbin"
  2 "--name"
  3 "fatbinData"
  4 "--section"
  5 "__CUDAFATBINDATASECTION"
  6 "--c-kind=win32"
  7 "--type"
  8 "longlong"
  9 "--static"
  10 "--const"
*/
int main(int argc, char* argv[])
{
    int i;
    fprintf(stderr, "bin2c<<<\n");
    for(i=1; i<argc; i++)
    {
        fprintf(stderr, "  %d \"%s\"\n", i, argv[i]);
    }
    fprintf(stderr, ">>>\n");
    return 0;
}


