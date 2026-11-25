/*   Foma: a finite-state toolkit and library.                                 */
/*   Copyright © 2008-2021 Mans Hulden                                         */

/*   This file is part of foma.                                                */

/*   Licensed under the Apache License, Version 2.0 (the "License");           */
/*   you may not use this file except in compliance with the License.          */
/*   You may obtain a copy of the License at                                   */

/*      http://www.apache.org/licenses/LICENSE-2.0                             */

/*   Unless required by applicable law or agreed to in writing, software       */
/*   distributed under the License is distributed on an "AS IS" BASIS,         */
/*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  */
/*   See the License for the specific language governing permissions and       */
/*   limitations under the License.                                            */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "foma.h"

/* Both stacks start small and double in size when full, */
/* so their size is limited only by available memory.    */
#define INITIAL_STACK_SIZE 1024

static int *a = NULL;
static int a_size = 0;
static int top = -1;

static void **ptr_stack = NULL;
static int ptr_stack_size = 0;
static int ptr_stack_top = -1;

/* Grows *buf to hold at least needed elements of elem_size bytes. */
static void stack_grow(void **buf, int *size, size_t elem_size, int needed, const char *name) {
    int newsize;
    void *newbuf;
    newsize = *size == 0 ? INITIAL_STACK_SIZE : *size;
    while (newsize < needed) {
        if (newsize > INT_MAX / 2) {
            fprintf(stderr, "%s full!\n", name);
            exit(1);
        }
        newsize *= 2;
    }
    newbuf = realloc(*buf, (size_t) newsize * elem_size);
    if (newbuf == NULL) {
        fprintf(stderr, "%s full!\n", name);
        exit(1);
    }
    *buf = newbuf;
    *size = newsize;
}

int ptr_stack_isempty() {
    return ptr_stack_top == -1;
}

void ptr_stack_clear() {
    ptr_stack_top = -1;
}

void *ptr_stack_pop() {
    return ptr_stack[ptr_stack_top--];
}

int ptr_stack_isfull() {
    return 0;
}

void ptr_stack_push(void *ptr) {
    if (ptr_stack_top + 1 >= ptr_stack_size)
        stack_grow((void **) &ptr_stack, &ptr_stack_size, sizeof(void *), ptr_stack_top + 2, "Pointer stack");
    ptr_stack[++ptr_stack_top] = ptr;
}


int int_stack_isempty() {
  return top == -1;
}

void int_stack_clear() {
  top = -1;
}

int int_stack_find (int entry) {
  int i;
  if (int_stack_isempty()) {
    return 0;
  }
  for(i = 0; i <= top ; i++) {
    if (entry == a[i]) {
      return 1;
    }
  }
  return 0;
}

int int_stack_size () {
  return (top + 1);
}

void int_stack_push(int c) {
  if (top + 1 >= a_size)
    stack_grow((void **) &a, &a_size, sizeof(int), top + 2, "Stack");
  a[++top] = c;
}


int int_stack_pop() {
  return a[top--];
}

int int_stack_isfull() {
  return 0;
}
