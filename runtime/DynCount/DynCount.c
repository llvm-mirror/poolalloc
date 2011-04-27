#include <stdio.h>
#include <stdlib.h>

static unsigned long * Total = 0;
static unsigned long * Safe = 0;

static void
printItAll (void) {
  FILE * fp = fopen ("lsstats", "w");
  fprintf (fp, "%lu Safe \n", *Safe);
  fprintf (fp, "%lu Total \n", *Total);
  fflush (fp);
  fclose (fp);
  return;
}

void
DYN_COUNT_setup (unsigned long * total, unsigned long * safe) {
  Total = total;
  Safe = safe;
  atexit (printItAll);
  return;
}


