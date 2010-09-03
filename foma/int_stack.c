/*     Foma: a finite-state toolkit and library.                             */
/*     Copyright © 2008-2009 Mans Hulden                                     */

/*     This file is part of foma.                                            */

/*     Foma is free software: you can redistribute it and/or modify          */
/*     it under the terms of the GNU General Public License version 2 as     */
/*     published by the Free Software Foundation. */

/*     Foma is distributed in the hope that it will be useful,               */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/*     GNU General Public License for more details.                          */

/*     You should have received a copy of the GNU General Public License     */
/*     along with foma.  If not, see <http://www.gnu.org/licenses/>.         */

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

/*
int int_stack_cmp (const void *a, const void *b) {
  return ( *(int*)b - *(int*)a );
}

void int_stack_sort() {
  if (top)
    qsort(a, top+1, sizeof(int), int_stack_cmp);
}
*/

/* 
  printf("Before: ");
  for (j=top; j >=0; j--)
    printf("[%i]",a[j]);
  
  printf("After: ");
  for (j=top; j >=0; j--)
    printf("[%i]",a[j]);
  printf("\n");
*/

/* Insertion sort. Best for small arrays (<20) */
/*
void int_stack_sort() {
  
  register int j, p;
  register int tmp;
    
  if (top > 0) {
    for(p=1; p<(top+1); p++) {
      tmp = a[p];
      for(j = p; j > 0 && a[j-1] < tmp; j--)
	a[j] = a[j-1];
      a[j] = tmp;
    }
  }
}
*/
/* Shellsort implementation. Probably too much overhead for small arrays */

void int_stack_sort() {

  int i, j, k, h, v;
  int cols[16] = {1391376, 463792, 198768, 86961, 33936, 13776, 4592, 1968, 861, 336, 112, 48, 21, 7, 3, 1};
  /* printf("Size: [%i]", top+1); */
  if (top<1) {
    return;
  }

  for (k=0; k<16; k++) {
    h=cols[k];
    for (i=h; i<(top+1); i++) {
      v=a[i]; j=i;
      while (j >= h && a[j-h] < v) {
	a[j]=a[j-h];
	j=j-h;
      }
      a[j]=v;
    }
  }
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

int int_stack_status() {
  return top;
}

int int_stack_isfull() {
  return (top == (MAX_STACK - 1));
}

