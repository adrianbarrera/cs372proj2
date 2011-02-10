#ifndef _SORTEDPOINTS_H_
#define _SORTEDPOINTS_H_
#include "ULT.h"


typedef struct ReadyListStruct {
  struct Node *hd;
} SortedPoints;
  
typedef struct Node{
  struct Node *next;
  ThrdCtlBlk *tcb;
  Tid tid;
} Node;

ReadyList *rl_init(ReadyList *rl);

int add(ReadyList *rl, ThrdCtlBlk *tcb);

ThrdCltBlk * first(ReadyList *rl);

ThrdCtlBlk * removeTid(ReadyList *rl, Tid tid);

#endif
