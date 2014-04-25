#include "dldl.h"

DLDL *dldl_create() {
  DLDL *list = malloc(sizeof(DLDL));
  
  list->first = NULL;
  list->last = NULL;

  return list;
}

int dldl_destroy(DLDL *list) {
  DLDLNode *current;

  for (current = list->first; current != NULL; current = current->next) {
    if (current->prev) {
      free(current->prev);
    }
  }
  free(list->last);
  free(list);

  return 1;
}

int dldl_push(DLDL *list, void *element) {  
  DLDLNode *node = malloc(sizeof(DLDLNode));

  if (!node) {
    return -1;
  }

  node->element = element;
  node->next = NULL;
  node->prev = NULL;

  if (list->last == NULL) {
    list->first = node;
    list->last = node;
  } else {
    list->last->next = node;
    node->prev = list->last;
    list->last = node;
  }

  list->size++;

  return 1;
}

void *dldl_pop(DLDL *list) {
  DLDLNode *node = list->last;
  return node != NULL ? dldl_remove(list, node) : NULL;
}

void *dldl_shift(DLDL *list) {
  DLDLNode *node = list->first;
  return node != NULL ? dldl_remove(list, node) : NULL;
}

void *dldl_remove(DLDL *list, DLDLNode *node) {
  void *element = NULL;

  if (list->first == NULL || list->last == NULL) {
    perror("Empty list.");
    errno = 0;
    return NULL;
  }

  if (node == NULL) {
    perror("Null node.");
    errno = 0;
    return NULL;
  }

  if (node == list->first && node == list->last) {
    list->first = NULL;
    list->last = NULL;
  } else if (node == list->first) {
    list->first = node->next;
    list->first->prev = NULL;
  } else if (node == list->last) {
    list->last = node->prev;
    list->last->next = NULL;
  } else {
    DLDLNode *next = node->next;
    DLDLNode *prev = node->prev;

    next->prev = prev;
    prev->next = next;
  }

  list->size--;
  element = node->element;
  free(node);

  return element;
}
