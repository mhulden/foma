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
  stack_ptr->amedh = NULL;
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

struct apply_med_handle *stack_get_med_ah() {
    struct stack_entry *se;
    se = stack_find_top();
    if (se == NULL) {
	return(NULL);
    }
    if (se->amedh == NULL) {
	se->amedh = apply_med_init(se->fsm);
	apply_med_set_align_symbol(se->amedh, "-");
    }
    return(se->amedh);
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

struct fsm *stack_pop(void) {
  int i;
  struct fsm *fsm;
  struct stack_entry *stack_ptr;
  if (stack_size() == 1) {
    fsm = main_stack->fsm;
    main_stack->fsm = NULL;
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
  if (stack_ptr->amedh != NULL) {
      apply_med_clear(stack_ptr->amedh);
      stack_ptr->amedh = NULL;
  }
  stack_ptr->fsm = NULL;
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

int stack_clear(void) {
  struct stack_entry *stack_ptr;
  for (stack_ptr = main_stack ; stack_ptr->next != NULL; stack_ptr = main_stack) {
    if (stack_ptr->ah != NULL)
	apply_clear(stack_ptr->ah);
    if (stack_ptr->amedh != NULL)
	apply_med_clear(stack_ptr->amedh);

    main_stack = stack_ptr->next;
    fsm_destroy(stack_ptr->fsm);
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
