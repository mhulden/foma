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

#include "fomalib.h"

#define STRING_HASH_SIZE 8191

unsigned int sh_hashf(char *string);

struct sh_handle *sh_init() {
    struct sh_handle *sh;
    sh = xxmalloc(sizeof(struct sh_handle));
    sh->hash = xxcalloc(STRING_HASH_SIZE, sizeof(struct sh_hashtable));
    return(sh);
}

void sh_done(struct sh_handle *sh) {
    int i;
    struct sh_hashtable *hash, *hashp;
    for (i=0; i < STRING_HASH_SIZE; i++) {
	hash = sh->hash + i;
	if (hash->string != NULL)
	    xxfree(hash->string);
	for (hash=hash->next ; hash != NULL ; hash = hashp) {
	    hashp = hash->next;
	    if (hash->string != NULL)
		xxfree(hash->string);
	    xxfree(hash);
	}
    }
    xxfree(sh->hash);
    xxfree(sh);
}

int sh_get_value(struct sh_handle *sh) {
    return(sh->lastvalue);
}

char *sh_find_string(struct sh_handle *sh, char *string) {
    struct sh_hashtable *hash;
    for (hash = sh->hash + sh_hashf(string) ; hash != NULL; hash = hash->next) {
	if (hash->string == NULL)
	    return NULL;
	if (strcmp(hash->string, string) == 0) {
	    sh->lastvalue = hash->value;
	    return(hash->string);
	}
    }
    return NULL;
}

char *sh_find_add_string(struct sh_handle *sh, char *string, int value) {
    char *s;
    s = sh_find_string(sh, string);
    if (s == NULL)
	return (sh_add_string(sh, string, value));
    else
	return(s);
}

char *sh_add_string(struct sh_handle *sh, char *string, int value) {
    struct sh_hashtable *hash, *newhash;
    
    hash = sh->hash + sh_hashf(string);
    if (hash->string == NULL) {
	hash->string = xxstrdup(string);
	hash->value = value;
	return(hash->string);
    } else {
	newhash = xxmalloc(sizeof(struct sh_hashtable));
	newhash->string = xxstrdup(string);
	newhash->value = value;
	newhash->next = hash->next;
	hash->next = newhash;
	return(newhash->string);
    }
}

unsigned int sh_hashf(char *string) {
    register unsigned int hash;
    hash = 0;
    
    while (*string != '\0') {
        hash = hash * 101  +  *string++;
    }
    return (hash % STRING_HASH_SIZE);
}
