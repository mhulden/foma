/*     Foma: a finite-state toolkit and library.                             */
/*     Copyright © 2008-2010 Mans Hulden                                     */

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
#include <string.h>
#include <stdlib.h>
#include "foma.h"

extern int quiet_mode;

struct stack_entry *main_stack;

int stack_size() {
  int i;
  struct stack_entry *stack_ptr;
  for (i=0, stack_ptr = main_stack; stack_ptr->next != NULL; i++)
    stack_ptr = stack_ptr->next;
  return i;
}

int stack_init() {
  main_stack = xxmalloc(sizeof(struct stack_entry));
  main_stack->number = -1;
  main_stack->fsm = NULL;
  main_stack->next = NULL;
  main_stack->previous = NULL;
  return 1;
}

int stack_add(struct fsm *fsm) {
  int i;
  struct stack_entry *stack_ptr, *stack_ptr_previous;
  stack_ptr_previous = NULL;

  fsm_count(fsm);
  if (strcmp(fsm->name,"") == 0) 
      sprintf(fsm->name, "%X",rand());
  for (i=0, stack_ptr = main_stack; stack_ptr->number != -1; i++) {
    stack_ptr_previous = stack_ptr;
    stack_ptr = stack_ptr->next;
  }
  stack_ptr->next = xxmalloc(sizeof(struct stack_entry));
  stack_ptr->fsm = fsm;
  stack_ptr->ah = NULL;
  stack_ptr->number = i;
  stack_ptr->previous = stack_ptr_previous;
  (stack_ptr->next)->number = -1;
  (stack_ptr->next)->fsm = NULL;
  (stack_ptr->next)->next = NULL;
  (stack_ptr->next)->previous = stack_ptr;
  if (!quiet_mode)
      print_stats(fsm);
  return(stack_ptr->number);
}

struct fsm *stack_pop_q () {
  struct fsm *fsm;
  struct stack_entry *stack_ptr;
  if (stack_size() == 1) {
    fsm = main_stack->fsm;
    stack_clear();
    return(fsm);
  }
  stack_ptr = main_stack;
  fsm = main_stack->fsm;
  main_stack = main_stack->next;
  if (stack_ptr->ah != NULL) {
      apply_clear(stack_ptr->ah);
      stack_ptr->ah = NULL;
  }
  xxfree(stack_ptr);
  return(fsm);
}

struct apply_handle *stack_get_ah() {
    struct stack_entry *se;
    se = stack_find_top();
    if (se == NULL) {
	return(NULL);
    }
    if (se->ah == NULL) {
	se->ah = apply_init(se->fsm);
    }
    return(se->ah);
}

struct fsm *stack_pop () {
  int i;
  struct fsm *fsm;
  struct stack_entry *stack_ptr;
  if (stack_size() == 1) {
    fsm = main_stack->fsm;
    stack_clear();
    return(fsm);
  }
  for (i=0, stack_ptr = main_stack; (stack_ptr->next)->number != -1; stack_ptr = stack_ptr->next, i++);
  (stack_ptr->previous)->next = stack_ptr->next;
  (stack_ptr->next)->previous = stack_ptr->previous;
  fsm = stack_ptr->fsm;
  if (stack_ptr->ah != NULL) {
      apply_clear(stack_ptr->ah);
      stack_ptr->ah = NULL;
  }
  xxfree(stack_ptr);
  return(fsm);
}

int stack_isempty () {
  if (main_stack->next == NULL) {
    return 1;
  } else {
    return 0;
  }
}

int stack_turn () {
  struct stack_entry *stack_ptr;
  if (stack_isempty()) {
    printf("Stack is empty.\n");
    return 0;
  }
  if (stack_size() == 1) {
    return 1;
  }

  stack_ptr = stack_find_top();
  main_stack->next = stack_ptr->next;
  (stack_ptr->next)->previous = main_stack;
  main_stack = stack_ptr;

  while (stack_ptr->previous != NULL) {
    stack_ptr->next = stack_ptr->previous;
    stack_ptr = stack_ptr->next;
  }
  for (stack_ptr = main_stack; stack_ptr->number != -1;) {
    (stack_ptr->next)->previous = stack_ptr;
  }
  return 1;
}

struct stack_entry *stack_find_top () {
  struct stack_entry *stack_ptr;
  if (main_stack->number == -1) {
    return NULL;
  }
  for (stack_ptr = main_stack; (stack_ptr->next)->number != -1; stack_ptr = stack_ptr->next);
  return(stack_ptr);
}

struct stack_entry *stack_find_bottom () {
  if (main_stack->number == -1) {
    return NULL;
  }
  return(main_stack);
}

struct stack_entry *stack_find_second () {
  struct stack_entry *stack_ptr;
  /*  
      if (main_stack->number == -1) {
      return NULL;
      }
  */
  for (stack_ptr = main_stack; (stack_ptr->next)->number != -1; stack_ptr = stack_ptr->next);
  return(stack_ptr->previous);
}

int stack_clear () {
  struct stack_entry *stack_ptr;
  for (stack_ptr = main_stack ; stack_ptr->next != NULL; stack_ptr = main_stack) {
    if (stack_ptr->ah != NULL)
      apply_clear(stack_ptr->ah);

    main_stack = stack_ptr->next;
    xxfree(stack_ptr);
  }
  xxfree(stack_ptr);
  return(stack_init());
}

int stack_rotate () {

  /* Top element of stack to bottom */
  struct stack_entry *stack_ptr;
  struct fsm *temp_fsm;

  if (stack_isempty()) {
    printf("Stack is empty.\n");
    return -1;
  }
  if (stack_size() == 1) {
    return 1;
  }
  stack_ptr = stack_find_top();
  temp_fsm = main_stack->fsm;
  main_stack->fsm = stack_ptr->fsm;
  stack_ptr->fsm = temp_fsm;
  return 1;
}

int stack_print () {
  return 1;
}
