#ifndef LIST_H 
#define LIST_H

#include <stdlib.h>
#include <stdio.h>

typedef struct pair {
   int task_id;
   int pid;
} pair;

struct node {
   pair *val;
   struct node *next;
};

typedef struct node node;


typedef struct list {
   node *head;
   node *tail;
} list;

void ladd(list *l, pair *p);

void lremove(list *l, pair *p);

list* lcreate();

void lprint(list *l);


#endif