#include "kernel/types.h"
#include "user/user.h"

#define N 35

void child(int* left){
    
    int out;
    // 先尝试从left中读一个数，如果读不到说明已经结束
    close(left[1]);
    if(read(left[0],&out,sizeof(int))==0){
      exit(0);
    }
    // 否则再fork一个新的进程，将筛选后的素数写到right中
    int right[2];
    pipe(right);
    if(fork()==0){
      child(right);
    }else{
      printf("prime %d\n",out);
      close(left[1]);
      int out2;
      while(read(left[0],&out2,sizeof(int))!=0){
        if(out2%out!=0){
          close(right[0]);
          write(right[1],&out2,sizeof(int));
        }
      }
      close(right[1]);
      wait(0);
    }
    exit(0);
    
}

int
main(int argc, char* argv[]) {
  
  int left[2];
  pipe(left);
  if(fork()==0){
    child(left);
  }else {
    close(left[0]);
    for (int i=2; i < N;i++) {
      write(left[1],&i,sizeof(int));
    }
    close(left[1]);
    wait(0);
  }
  exit(0);
}

  // for (;i < N;i++) {
    
  //   pipe(right);
  //   close(left[0]);
  //   write(left[1], i, sizeof(int));

    
    
  //     close(left[1]);
  //     read(left[0], out, sizeof(int));
  //     printf("prime %d\n",out);

  //   }

    // if (isprime[i]==0) {
    //   printf("prime %d\n",i);
    //   int j;
    //   for (j = i * i;j <= N;j += i)//比如说i=5,5*4,5*3.5*2在前面都已经被2*5，3*5，4*5除去了
    //     //因为小于 i 的所有的倍数都被筛过，所以直接从 i*i 开始，从这里也可以看出，筛素数时到 N^0.5 就可以了
    //     isprime[j] = 1;
    // }
