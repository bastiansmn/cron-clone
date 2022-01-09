#include "list.h"


void ladd(list *l, pair *p) {
   node *n = malloc(sizeof(node));
   n->val = p;
   n->next = NULL;
   if (l->head == NULL) {
      l->head = n;
      l->tail = n;
   } else {
      l->tail->next = n;
      l->tail = n;
   }
}

void lremove(list *l, pair *p) {
   node *n = l->head;
   node *prev = NULL;
   while (n != NULL) {
      if (n->val->task_id == p->task_id) {
         if (prev == NULL) {
            l->head = n->next;
         } else {
            prev->next = n->next;
         }
         if (n->val->task_id == l->tail->val->task_id) {
            l->tail = prev;
         }
         free(n);
         return;
      }
      prev = n;
      n = n->next;
   }
}

list* lcreate() {
   list *l = malloc(sizeof(list));
   l->head = NULL;
   l->tail = NULL;
   return l;
}

void lprint(list *l) {
   node *n = l->head;
   printf("[");
   while (n != NULL) {
      printf(" (id: %d, pid: %d);", n->val->task_id, n->val->pid);
      n = n->next;
   }
   printf(" ]\n");
}