#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char* argv[]) {
  int p[2];
  char buf[1];
  pipe(p); //父进程写，子进程读
  /**
   * @brief Construct a new if object
   * 父进程先write，然后子进程read（等待write），printf，
   * 然后子进程再write，父进程收到后read，printf
   * 
   */
  // 不需要两个pipe
  // 读写完毕最后要close掉fd，这样最保险
  if (fork() == 0) {
    close(p[1]);
    read(p[0], buf, sizeof(buf));
    printf("%d: received ping\n",getpid());
    write(p[1], " ", 1);
    close(p[0]);
  }  else {
    close(p[0]);
    write(p[1], " ", 1);
    wait(0);
    read(p[0], buf, sizeof(buf));
    printf("%d: received pong\n",getpid());
    close(p[1]);
  }


  //只要保证read可以执行完毕即可，即能读到数据或者写端fd全部被关闭，读到EOF
  // if (fork() == 0) {
  //   read(p[0], buf, sizeof(buf));
  //   printf("%d: received ping\n",getpid());
  //   write(p[1], " ", 1);
  // }  else {
  //   write(p[1], " ", 1);
  //   read(p[0], buf, sizeof(buf));
  //   printf("%d: received pong\n",getpid());
  // }

  exit(0);
}
