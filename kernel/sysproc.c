#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if (myproc()->killed) {
      release(&tickslock);
      return -1;
    }
    backtrace();
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_sigalarm(void) {
  int n;
  uint64 fn;
  if (argint(0, &n) < 0 || argaddr(1, &fn) < 0)
    return -1;
  // printf("%d\n", fn);
  struct proc* p = myproc();
  // p->ticks = ticks;
  p->handler = (void(*)())fn;
  p->interval = n;
  return 0;
}

uint64
sys_sigreturn(void) {
  struct proc* p = myproc();
  p->in_handler = 0;
  p->trapframe->epc = p->save_epc;
  p->trapframe->ra=p->save_ra;
  p->trapframe->sp=p->save_sp;
  p->trapframe->gp=p->save_gp;
  p->trapframe->tp=p->save_tp;
  p->trapframe->t0=p->save_t0;
  p->trapframe->t1=p->save_t1;
  p->trapframe->t2=p->save_t2;
  p->trapframe->s0=p->save_s0;
  p->trapframe->s1=p->save_s1;
  p->trapframe->a0=p->save_a0;
  p->trapframe->a1=p->save_a1;
  p->trapframe->a2=p->save_a2;
  p->trapframe->a3=p->save_a3;
  p->trapframe->a4=p->save_a4;
  p->trapframe->a5=p->save_a5;
  p->trapframe->a6=p->save_a6;
  p->trapframe->a7=p->save_a7;
  p->trapframe->s2=p->save_s2;
  p->trapframe->s3=p->save_s3;
  p->trapframe->s4=p->save_s4;
  p->trapframe->s5=p->save_s5;
  p->trapframe->s6=p->save_s6;
  p->trapframe->s7=p->save_s7;
  p->trapframe->s8=p->save_s8;
  p->trapframe->s9=p->save_s9;
  p->trapframe->s10 = p->save_s10;
  p->trapframe->s11 = p->save_s11;
  p->trapframe->t3=p->save_t3;
  p->trapframe->t4=p->save_t4;
  p->trapframe->t5=p->save_t5;
  p->trapframe->t6=p->save_t6;
  return 0;
}
