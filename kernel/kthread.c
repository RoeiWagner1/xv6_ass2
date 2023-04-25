#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

extern struct proc proc[NPROC];
extern void forkret(void);

void kthreadinit(struct proc *p)
{

  initlock(&p->threadsId_lock, "proc");

  for (struct kthread *kt = p->kthread; kt < &p->kthread[NKT]; kt++)
  {
    initlock(&kt->lock, "kthread");
    kt->state = TUNUSED;
    kt->process = p;
    // WARNING: Don't change this line!
    // get the pointer to the kernel stack of the kthread
    kt->kstack = KSTACK((int)((p - proc) * NKT + (kt - p->kthread)));
  }
}


struct kthread *mykthread()
{
  push_off();
  struct cpu *c = mycpu();
  struct kthread *t = c->thread;
  pop_off();
  return t;
}


struct trapframe *get_kthread_trapframe(struct proc *p, struct kthread *kt)
{
  return p->base_trapframes + ((int)(kt - p->kthread));
}


int alloctid(struct proc* p)
{
  acquire(&p->threadsId_lock);
  int tid = p->threadsId;
  p->threadsId++;
  release(&p->threadsId_lock);
  return tid;
}

/**
 * allocates a thread
 * @post: returned thread's lock is acquired
*/
struct kthread*
allocthread(struct proc* p)
{
  struct kthread *kt;
  for (kt = p->kthread; kt < &p->kthread[NKT]; kt++) {
    acquire(&kt->lock);
    if(kt->state == TUNUSED) {
      goto found;
    }
    else {
      release(&kt->lock);
    }
  }
  return 0;

found:
  kt->tid = alloctid(p);
  kt->state = TUSED;
  get_kthread_trapframe(p, kt); 
  memset(&kt->context, 0, sizeof(kt->context));
  kt->context.ra = (uint64)forkret;
  kt->context.sp = kt->kstack + PGSIZE;
  return kt;
}

/**
 * resets thread fields
 * @pre: kt lock must be held
 * @post: kt lock remains held
*/
void
freethread(struct kthread* kt)
{
  kt->tid = 0;
  kt->chan = 0;
  kt->killed = 0;
  kt->xstate = 0;
  kt->process = 0;
  kt->kstack = 0;
  kt->trapframe = 0;

  kt->state = TUNUSED;
}
