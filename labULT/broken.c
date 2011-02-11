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
//Tid yret = -1;
//ReadyList *rl;

void switchThread(Tid wantTid);
int first();
Tid ULT_DestroyThread(Tid tid);
int getNextTid();
//void freeZombies();

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
  runningThread->yret = -1;
  tidLog[0] = 1;
  runningThread->zombie = 0;
}

Tid 
ULT_CreateThread(void (*fn)(void *), void *parg)
{
  if(init == 0) {
    initialize();
  }
 
  //freeZombies();
 
  // Check if we have too many threads
  if(readySize == 1023)
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
  newThread->stk = stack;  
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
  newThread->yret = -1;
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
  
  //freeZombies();
  //printf("ready[wantTid]: %p\n", ready[wantTid]);
  
  //printf("We are in yield.\n");
  
  if(wantTid < -2 || wantTid > 1023) {
    //printf("We are in the here\n");
	runningThread->yret = ULT_INVALID;
  } else {
    switchThread(wantTid);
  }
  
  return runningThread->yret;
}


Tid ULT_DestroyThread(Tid tid)
{
  if(init == 0) {
    initialize();
  }
  
  if(tid == runningThread->tid) {
    tid = ULT_SELF;
  }
  
  if(tid < -2 || tid > 1023) {
   printf("We are in the here\n");
	return ULT_INVALID;
  }
  
  if(tid == ULT_ANY) {
   printf("We are in the any\n");
    if(readySize == 0) {
		return ULT_NONE;
	}
	
	int i = first();
//	if(i == runningThread->tid){
		ready[i]->zombie = 1;
//	}
//	else{
//		free(ready[i]->context);
//		free(ready[i]->stk);
//		free(ready[i]);
//	}
		
	readySize--;
	return i;
  }
  
  //if self, mark self and pop ready
  if(tid == ULT_SELF) {
   printf("We are in the self\n");
	  int r = runningThread->tid;
	  ready[r]->zombie = 1;
	  readySize--;
	  ULT_Yield(ULT_ANY);
	  return r;
  }
  
  
  //else mark ready[tid]->zombie = 1
   printf("We are in the else\n");
   printf("ready[tid]: %d\n", ready[tid]->tid);
  //free(ready[tid]->context);
  //free(ready[tid]->stk);
  //free(ready[tid]);
  //ready[tid] = NULL;
   //printf("ready[tid]: %d\n", ready[tid]->tid);
  readySize--;
  return tid;
}

void switchThread(Tid wantTid)
{
  volatile int doneThat = 0;
  
  getcontext(runningThread->context);
  //printf("Done that for thread %d is: %d\n", runningThread->tid, doneThat);
  if(doneThat == 0)
  {
    doneThat = 1; 

    if(wantTid == runningThread->tid) {
      wantTid = ULT_SELF;
    }

    //choose new thread to run
    if(wantTid == ULT_SELF) {
	  //printf("We are in the ULT_SELF\n");
      runningThread->yret = runningThread->tid;
      setcontext(runningThread->context);
    }
    if(wantTid == ULT_ANY) {
      printf("We are in the ULT_ANY\n");
      if(readySize == 0) {
		  runningThread->yret = ULT_NONE;
		  setcontext(runningThread->context);
      }
      int oldTid = runningThread->tid;
      ready[runningThread->tid] = runningThread;
      int i = first();
      runningThread = ready[i];
      ready[runningThread->tid] = NULL;
      ready[oldTid]->yret = runningThread->tid;
      setcontext(runningThread->context);
    }
    
    if(ready[wantTid] == NULL || ready[wantTid]->zombie == 1) {
	  runningThread->yret = ULT_INVALID;
    } else {
      int oldTid = runningThread->tid;
      ready[runningThread->tid] = runningThread;
      //printf("We are in the ELSE\n");
      runningThread = ready[wantTid];
      ready[runningThread->tid] = NULL;
      ready[oldTid]->yret = runningThread->tid;
      setcontext(runningThread->context);
    }
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

void freeZombies()
{
  int i;
  for(i = 0; i < 1024; i++){
    if(ready[i] != NULL && i != runningThread->tid && ready[i]->zombie == 1){
	free(ready[i]->context);
	free(ready[i]);
    }
  }	
}
