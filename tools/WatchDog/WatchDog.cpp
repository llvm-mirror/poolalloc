//===-- watchdog - Watch Dog Tool -----------------------------------------===//
//
//                     The SAFECode Project
//
// This file was developed by the LLVM research group and is distributed
// under the University of Illinois Open Source License. See LICENSE.TXT for
// details.
//
// This file is based on code developed by Terry Lambert's mailing list post:
// http://lists.apple.com/archives/unix-porting/2005/Jun/msg00115.html.
//===----------------------------------------------------------------------===//
//
// This program executes another program and monitors its memory usage.  If the
// executed program consumes too much memory, then this program will terminate
// it.
//
// This program is needed on Mac OS X because memory limits set by setrlimit()
// are ignored by the kernel.  Instead, we must have a program carefully watch
// a program and terminate it if it uses too much memory.
//
//===----------------------------------------------------------------------===//

#include <cstdio>
#include <cstdlib>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace std;

// Process ID to watch.  This is global because it is needed by the signal
// handler.
static int pid_to_watch = 0;

// Number of seconds to wait before checking up on the child process
static const unsigned check_interval = 10;

// Maximum amount for the resident set size in kilobytes (i.e., how much
// physical memory is in use).
static const int rss_max_allowable = 4 * 1024 * 1024;

//
// Function: create_process()
//
// Description:
//  Create a new child process running the specified program.
//
// Return value:
//  0 - The program was not successfully created.
//  Otherwise, the process ID (PID) of the new process is returned.
//
static pid_t
create_process (int argc, char ** argv) {
  // Child process ID
  pid_t child;

  //
  // Create a new process.
  //
  switch (child = fork()) {
    // Parent
    default:
      return child;

    // Error
    case -1:
      return 0;

    // Child
    case 0:
      break;
  }

  //
  // We are now executing in the child process.  Do an exec()!
  //

  //
  // The exec() failed.
  //
  execvp (argv[1], &(argv[1]));
  fprintf (stderr, "Failed to execute program\n");
  exit (1);
}

#if defined(__APPLE__)
//
// Function: check_process()
//
// Description:
//  This function is a signal handler.  It is called on receipt of an alarm
//  signal to check the memory usage of a child process.
//
static void
check_process (int sig) {
  char cmd[256];
  int rss;
  FILE *fp;

  //
  // If we caught a signal, check on the child process's memory usage.
  //
  snprintf(cmd, 255, "ps -p %d -o rss | tail -1", pid_to_watch);
  if ((fp = popen(cmd, "r")) != NULL) {
    if (fscanf(fp, "%d", &rss) == 1) {
      if (rss > rss_max_allowable) {
        fprintf (stderr, "WatchDog: Terminating bad process %d!\n", pid_to_watch);
        kill (pid_to_watch, SIGKILL);
        return;
      }
    }
    pclose(fp);
  }

  //
  // Reset the alarm.
  //
  signal (SIGALRM, check_process);
  alarm (check_interval);
  return;
}
#endif

int
main (int argc, char ** argv) {
  //
  // Verify that we have sufficient command line arguments.
  //
  if (argc < 2) {
    fprintf (stderr, "Usage: %s <command> [args ...]\n", argv[0]);
    exit (1);
  }

  //
  // Execute the program to watch.
  //
  pid_to_watch = create_process (argc, argv);
  if (!pid_to_watch) {
    fprintf (stderr, "Failed to create child process\n");
    exit (1);
  }

  //
  // Set up an alarm to wake us up.
  //
#if defined(__APPLE__)
  signal (SIGALRM, check_process);
  alarm (check_interval);
#endif

  //
  // Keep looping until the child terminates or we terminate it.
  //
  int status;
  waitpid (pid_to_watch, &status, 0);

  //
  // The program has terminated.  Get its exit status and use that as our
  // exit status.
  //
  exit (WEXITSTATUS (status));
}

