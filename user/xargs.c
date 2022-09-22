#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

#define ARG_MAXLEN 100

int
main(int argc, char* argv[]) {

  char input;
  char* command = argv[1];
  // 因为不知道输入的参数有多少个，所以只能开最大
  char argvBackup[MAXARG][ARG_MAXLEN];


  char* exec_arg[MAXARG];

  // while (1) {
    int arg_num = argc - 1;
    int cur = 0;
    memset(argvBackup, 0, MAXARG * ARG_MAXLEN);
    // printf("%s\n", argv[1]);
    // 因为要保证argv的完整性，新开的argvBackup也要从argv[1]开始，
    // 这样argvBackup[0]才是目标命令本身
      for (int i = 1; i < argc; i++) {
      strcpy(argvBackup[i - 1], argv[i]);
    }


    int read_result;
    while ((read_result = read(0, &input, 1)) == 1 && input != '\n') {
      if (input != ' ') {
        argvBackup[arg_num][cur++] = input;
      }
      else {
        arg_num++;
        cur = 0;
      }
    }

    // 遇到EOF或者\n
    // if (read_result <= 0) {
    //   // printf("dddd\n");
    //   break;
    // }

    for (int i = 0; i <= arg_num; i++) {
      exec_arg[i] = argvBackup[i];
    }
    exec_arg[arg_num + 1] = 0;

    if (fork() == 0) {
      exec(command, (char**)exec_arg);
      // printf("%s 0-%s 1-%s 2-%s\n", command, exec_arg[0], exec_arg[1], exec_arg[2]);
      exit(0);
    }
    else {
      wait(0);
    }
  // }
  exit(0);
}
