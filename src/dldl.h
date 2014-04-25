#ifndef _DLDL_H_
#define _DLDL_H_

/**
 * Implementação de uma lista duplamente ligada.
 * Este código é fortemente baseado no exemplo explicado no link:
 * http://c.learncodethehardway.org/book/ex32.html
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

typedef struct dldl_node {
  struct dldl_node *prev;
  struct dldl_node *next;
  void *element;
} DLDLNode;

typedef struct dldl {
  size_t size;
  DLDLNode *first;
  DLDLNode *last;
} DLDL;

DLDL *dldl_create();
int dldl_destroy(DLDL *list);
int dldl_push(DLDL *list, void *element);
void *dldl_pop(DLDL *list);
void *dldl_shift(DLDL *list);
void *dldl_remove(DLDL *list, DLDLNode *node);


#endif
