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

// Each spot in the ready list points to a TCB or null
// Threads are indexed by their tid
ThrdCtlBlk * ready[1024];

// Used tidLog to indicate which spots/tids in the ready list are in use
int tidLog[1024];
int readySize = 0;
ThrdCtlBlk * runningThread;
int init = 0;

void switchThread(Tid wantTid);
int first();
Tid ULT_DestroyThread(Tid tid);
int getNextTid();
void freeZombies();

void stub(void (*root)(void *), void *arg)
{
  // thread starts here
  Tid ret;
  root(arg); // call root function
  ret = ULT_DestroyThread(ULT_SELF);
  assert(ret == ULT_NONE); // we should only get here if we are the last thread.
  exit(0); // all threads are done, so process should exit 
} 

/*
 * Need this function to create our first runningThread and clear the ready list
 */
void initialize()
{
  init = 1;

  runningThread = (ThrdCtlBlk *)malloc(sizeof(struct ThrdCtlBlk));
  runningThread->context = (struct ucontext *)malloc(sizeof(struct ucontext));
  
  int i = 0;

  // Clear out the ready list so that it isnt full of garbage
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

  //check if there are any zombies to be freed
  freeZombies();
  
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
  
  // Allocate stack and move to top
  int * stack = malloc(ULT_MIN_STACK);
  stack = stack + (ULT_MIN_STACK / 4);
  
  if(stack == NULL)
    return ULT_NOMEMORY;
    
  // Push arguments onto stack
  *stack = (unsigned int) parg;
  stack--;
  *stack = (unsigned int) fn;
  stack--;
  
  // Make stack pointer point to stack we created
  newThread->context->uc_mcontext.gregs[REG_ESP] = (unsigned int)stack;
  
  newThread->tid = getNextTid();
  newThread->yret = -1;
  ready[newThread->tid] = newThread;
  tidLog[newThread->tid] = 1;
  readySize++;
  
  return newThread->tid;
}



Tid ULT_Yield(Tid wantTid)
{
  if(init == 0) {
    initialize();
  }
  
  if(wantTid < -2 || wantTid > 1023) {
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
	return ULT_INVALID;
  }
  
  if(tid == ULT_ANY) {
    if(readySize == 0) {
		return ULT_NONE;
	}
	
	//select the first thread in the ready list to destroy
	int i = first();

	//if the first one is the running thread, make it a zombie
      if(i == runningThread->tid){
              ready[i]->zombie = 1;
      }
	//otherwise free the memory associated with the thread
      else{
              free(ready[i]->context);
              free(ready[i]->stk);
              free(ready[i]);
      }

      readySize--;
      return i;
  }
  
  // If self, mark self and pop ready
  if(tid == ULT_SELF) {
	  int r = runningThread->tid;
	  ready[r]->zombie = 1;
	  readySize--;
      // Can't free here but once the next runningThread has been picked we can
	  ULT_Yield(ULT_ANY);
	  return  r;
  }
  
  
  //otherwise destroy the thread with TID tid
  free(ready[tid]->context);
  free(ready[tid]->stk);
  free(ready[tid]);
  ready[tid] = NULL;
  readySize--;
  return tid;
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

    // Self case
    if(wantTid == ULT_SELF) {
      runningThread->yret = runningThread->tid;
      setcontext(runningThread->context);
    }
    
    // Any case
    if(wantTid == ULT_ANY) {
      if(readySize == 0) {
		  runningThread->yret = ULT_NONE;
		  setcontext(runningThread->context);
      }
      
      int oldTid = runningThread->tid;
      ready[runningThread->tid] = runningThread;
      
      // Get thread tid from ready list 
      int i = first();
      
      // Make the thread the runningThread and remove its TCB from ready list
      runningThread = ready[i];
      ready[runningThread->tid] = NULL;
      ready[oldTid]->yret = runningThread->tid;
      setcontext(runningThread->context);
    }
    
    // if thread exists on the ready and isn't dead, switch to it
    if(ready[wantTid] == NULL || ready[wantTid]->zombie == 1) {
	  runningThread->yret = ULT_INVALID;
    } else {
      int oldTid = runningThread->tid;
      ready[runningThread->tid] = runningThread;
      runningThread = ready[wantTid];
      ready[runningThread->tid] = NULL;
      ready[oldTid]->yret = runningThread->tid;
      setcontext(runningThread->context);
    }
  }
  return;
}

/*
 * Returns tid of an active thread on the ready list
 */
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

/*
 * Return available tid/free index on ready list
 */
int getNextTid()
{
  int i = 0;
  while(tidLog[i] == 1 && i < 1024) {
    i++;
  }
  
  return i;
}

/*
 * free any zombies on the ready list
 */
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


