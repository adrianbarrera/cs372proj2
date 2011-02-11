#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/* We want the extra information from these definitions */
#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */
#include <ucontext.h>

//#include "ReadyList.h"
#include "ULT.h"

ThrdCtlBlk * ready[1024];
int tidLog[1024];
int readySize = 0;
ThrdCtlBlk * runningThread;
int init = 0;
Tid yret = -1;
//ReadyList *rl;

void switchThread(Tid wantTid);
int first();
Tid ULT_DestroyThread(Tid tid);
int getNextTid();

void stub(void (*root)(void *), void *arg)
{
  // thread starts here
  Tid ret;
  root(arg); // call root function
  ret = ULT_DestroyThread(ULT_SELF);
  assert(ret == ULT_NONE); // we should only get here if we are the last thread.
  exit(0); // all threads are done, so process should exit 
} 

void initialize()
{
  init = 1;

  runningThread = (ThrdCtlBlk *)malloc(sizeof(struct ThrdCtlBlk));
  runningThread->context = (struct ucontext *)malloc(sizeof(struct ucontext));
  //runningThread->context->uc_stack.ss_sp = (void *)malloc(ULT_MIN_STACK);
  
  // will be calling getcontext later
  //getcontext(runningThread->context);
  //rl = (ReadyList *)malloc(sizeof(ReadyList));
  
  int i = 0;
  while(i < 1024) {
	  ready[i] = NULL;
	  i++;
  }
  
  runningThread->tid = 0;
  tidLog[0] = 1;
  runningThread->zombie = 0;
}

Tid 
ULT_CreateThread(void (*fn)(void *), void *parg)
{
  if(init == 0) {
    initialize();
  }
  
  // Check if we have too many threads
  if(readySize == 1024)
    return ULT_NOMORE;
  
  struct ucontext * cntxt;
  ThrdCtlBlk *newThread = (ThrdCtlBlk *)malloc(sizeof(ThrdCtlBlk));
  cntxt = (struct ucontext *)malloc(sizeof(struct ucontext));
  
  getcontext(cntxt);
  
  newThread->context = cntxt;
  
  // Change PC to point to stub function
  newThread->context->uc_mcontext.gregs[REG_EIP] = (unsigned int) &stub;
  
  // Allocate stack
  //newThread->context->uc_stack.ss_sp = (void *)malloc(ULT_MIN_STACK);
  int * stack = malloc(ULT_MIN_STACK);
  //printf("REG_ESP after malloc is at address: %p\n", stack);
  stack = stack + (ULT_MIN_STACK / 4);
  //printf("REG_ESP after adjust is at address: %p\n", stack);
  
  if(stack == NULL)
    return ULT_NOMEMORY;
    
  // move stack pointer and include arguments
  *stack = (unsigned int) parg;
  stack--;
  //printf("REG_ESP after moving to top is at address: %p\n", stack);
  *stack = (unsigned int) fn;
  stack--;
  //printf("REG_ESP after pushing args is at address: %p\n", stack);
  
  newThread->context->uc_mcontext.gregs[REG_ESP] = (unsigned int)stack;
  
  newThread->tid = getNextTid();
  ready[newThread->tid] = newThread;
  //if(ready[newThread->tid] == NULL);
  //printf("ready[newThread->tid]: %p\n", ready[newThread->tid]);
  tidLog[newThread->tid] = 1;
  readySize++;
  
  //printf("Tid of created thread: %d\n", ready[newThread->tid]->tid);
  return newThread->tid;
}



Tid ULT_Yield(Tid wantTid)
{
  if(init == 0) {
    initialize();
  }
  
  //printf("ready[wantTid]: %p\n", ready[wantTid]);
  
  //printf("We are in yield.\n");
  
  switchThread(wantTid);
  
  return yret;
}


Tid ULT_DestroyThread(Tid tid)
{
  if(init == 0) {
    initialize();
  }
  
  assert(0); /* TBD */
  return ULT_FAILED;
}

void switchThread(Tid wantTid)
{
  volatile int doneThat = 0;
  
  getcontext(runningThread->context);
  
  if(doneThat == 0)
  {
    doneThat = 1; 

    if(wantTid == runningThread->tid) {
      wantTid = ULT_SELF;
    }

    if(wantTid != ULT_ANY && wantTid != ULT_SELF) {
      if(wantTid < -2 || wantTid > 1023 || ready[wantTid] == NULL || ready[wantTid]->zombie == 1) {
		printf("We are in the here\n");
	    yret = ULT_INVALID;
	    setcontext(runningThread->context);
      }
    }
    //choose new thread to run
    if(wantTid == ULT_SELF) {
	  printf("We are in the ULT_SELF\n");
      yret = runningThread->tid;
      setcontext(runningThread->context);
    }
    if(wantTid == ULT_ANY) {
      printf("We are in the ULT_ANY\n");
      if(readySize == 0) {
		  yret = ULT_NONE;
		  setcontext(runningThread->context);
      }
      
      ready[runningThread->tid] = runningThread;
      int i = first();
      runningThread = ready[i];
      ready[runningThread->tid] = NULL;
      yret = runningThread->tid;
      setcontext(runningThread->context);
    }
    
    ready[runningThread->tid] = runningThread;
    printf("We are in the ELSE\n");
    runningThread = ready[wantTid];
    ready[runningThread->tid] = NULL;
    yret = runningThread->tid;
    setcontext(runningThread->context);
   }
  return;
}

int first()
{
  int j = runningThread->tid + 1;
  while(tidLog[j] == 0 && j < 1024){
    j++;
  }
  if(j == 1024) {
	  j = 0;
	  while(tidLog[j] == 0 && j < runningThread->tid){
	    j++;
      }
  }
  return j;
}

int getNextTid()
{
  int i = 0;
  while(tidLog[i] == 1 && i < 1024) {
    i++;
  }
  
  return i;
}
