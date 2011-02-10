#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "ULT.h"
#include "readyList.h"


ReadyList *rl_init(readyList *rl){
  rl->hd = NULL;
  return rl;
}

int add(ReadyList *rl, ThrdCtlBlk *tcb)
{
  struct Node *curr, *nw;
  nw = (struct Node *)malloc(sizeof(struct Node));
  if(nw == NULL)
    return 0;
  nw->tcb = tcb;
  nw->tid = tcb->tid;

  if(rl->hd == NULL){
        rl->hd = nw;
        nw->next = NULL;
  } else {
    curr = rl->head;
    while(curr->next != NULL) {
      curr = curr->next;
    }

    curr->next = nw;
  }
  return 1;
}

ThrdCtlBlk * first(ReadyList *rl)
{
  if(rl->hd == NULL)
	return NULL;

  else{
    temp = (rl->hd)->next;
    ret = (rl->hd)->tcb;
    free(rl->hd);
    rl->hd = temp;
  }
  return ret;
}

ThrdCtlBlk * removeTid(ReadyList *rl, Tid tid)
{
  struct Node *curr, *prev;
  ThrdCtlBlk *ret;
  if(rl->hd == NULL){
        return NULL;
  } else {
    curr = rl->hd;
    prev = NULL;
    while(curr->next != NULL) {
      if(curr->tid == tid) {
        prev->next = curr->next;
        ret = curr->tcb;
        free(curr);
      } else {
      prev = curr;
      curr = curr->next;
      }
    }
  }
  return ret;
}
