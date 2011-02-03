#include <assert.h>

/* We want the extra information from these definitions */
#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */
#include <ucontext.h>

#include "ULT.h"

ThrdCtlBlk * ready[1024];
int nextTid = 0;

void * switchThread(Tid wantTid);

Tid 
ULT_CreateThread(void (*fn)(void *), void *parg)
{
  assert(0); /* TBD */
  return ULT_FAILED;
}



Tid ULT_Yield(Tid wantTid)
{
	switch(wantTid);
}


Tid ULT_DestroyThread(Tid tid)
{
  assert(0); /* TBD */
  return ULT_FAILED;
}

void * switchThread(Tid wantTid)
{
  volatile int doneThat;
  
  struct ucontext *thisContext;
  ThrdCtlBlk *currentThread;
  ThrdCtlBlk *nextThread;
  //save state of current thread to TCB
  getcontext( &(thisContext));
  currentThread->context = thisContext;
  currentThread->tid = nextTid;

  ready[nextTid] = currentThread;

  if(!doneThat)
  {
    doneThat = 1;
	//choose new thread to run
    if(wantTid = ULT_SELF) {
		nextThread = currentThread;
	}

    nextTid++;
	setcontext(&(nextThread->context));
  }
}


