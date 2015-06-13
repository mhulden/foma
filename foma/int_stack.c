/*   Foma: a finite-state toolkit and library.                                 */
/*   Copyright © 2008-2015 Mans Hulden                                         */

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
#include "foma.h"

#define MAX_STACK 2097152
#define MAX_PTR_STACK 2097152

static int a[MAX_STACK];
static int top = -1;

static void *ptr_stack[MAX_PTR_STACK];
static int ptr_stack_top = -1;

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
    return (ptr_stack_top == (MAX_PTR_STACK - 1));
}

void ptr_stack_push(void *ptr) {
    if (ptr_stack_isfull()) {
        fprintf(stderr, "Pointer stack full!\n");
        exit(1);
    }
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
  if (int_stack_isfull()) {
    fprintf(stderr, "Stack full!\n");
    exit(1);
  }
  a[++top] = c;
}


int int_stack_pop() {
  return a[top--];
}

int int_stack_isfull() {
  return (top == (MAX_STACK - 1));
}
