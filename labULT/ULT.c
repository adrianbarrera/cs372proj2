#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/* We want the extra information from these definitions */
#ifndef __USE_GNU
#define __USE_GNU
#endif /* __USE_GNU */
#include <ucontext.h>

#include "ReadyList.h"
#include "ULT.h"

ThrdCtlBlk * ready[1024];
int tidLog[1024];
int readySize = 0;
ThrdCtlBlk * runningThread;
int nextTid = 0;
int init = 0;
Tid ret = -1;
ReadyList *rl;

void switchThread(Tid wantTid);
int firstFree();


void initialize()
{
  init = 1;

  runningThread = (ThrdCtlBlk *)malloc(sizeof(struct ThrdCtlBlk));
  runningThread->context = (struct ucontext *)malloc(sizeof(struct ucontext));
  
  // will be calling getcontext later
  //getcontext(runningThread->context);
  rl = (ReadyList *)malloc(sizeof(ReadyList));
  
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
  
  assert(0); /* TBD */
  return ULT_FAILED;
}



Tid ULT_Yield(Tid wantTid)
{
  if(init == 0) {
    initialize();
  }
  
  switchThread(wantTid);
  
  return ret;
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
	ret = ULT_INVALID;
	setcontext(runningThread->context);
      }
    }
    
    //choose new thread to run
    else if(wantTid == ULT_SELF) {
      ret = runningThread->tid;
      setcontext(runningThread->context);
    }
    
    else if(wantTid == ULT_ANY) {
      if(readySize == 0) {
	ret = ULT_NONE;
	setcontext(runningThread->context);
      }
      
      ready[runningThread->tid] = runningThead;
      
      int i = first();
      runningThread = ready[i];
      ready[runningThread->tid] = NULL;
      ret = runningThread->tid;
      setcontext(runningThread->context);
    }
   
    else {
      ready[runningThread->tid] = runningThead;
      
      runningThread = ready[wantTid];
      ready[runningThread->tid] = NULL;
      ret = runningThread->tid;
      setcontext(runningThread->context);
    }
  }
  
  return;
}

int first()
{
  int j = 0;
  while(tidLog[j] == 0 && j != runningThread->tid && j < 1024){
    j++;
  }
  return j;
}


