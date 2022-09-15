#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char* argv[]) {
  int p[2];
  int pp[2];
  char buf[1];
  pipe(p); //父进程写，子进程读
  pipe(pp);
  /**
   * @brief Construct a new if object
   * 父进程先write，然后子进程read（等待write），printf，
   * 然后子进程再write，父进程收到后read，printf
   * 
   */
  if (fork() == 0) {
    close(p[1]);
    close(pp[0]);
    read(p[0], buf, sizeof(buf));
    printf("%d: received ping\n",getpid());
    write(pp[1], " ", 1);
    close(p[0]);
    close(pp[1]);
  }  else {
    close(p[0]);
    close(pp[1]);
    write(p[1], " ", 1);
    read(pp[0], buf, sizeof(buf));
    printf("%d: received pong\n",getpid());
    close(p[1]);
    close(pp[0]);
  }
  exit(0);
}
