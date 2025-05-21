/**
 * Vector implementation.
 *
 * - Implement each of the functions to create a working growable array (vector).
 * - Do not change any of the structs
 * - When submitting, You should not have any 'printf' statements in your vector 
 *   functions.
 *
 * IMPORTANT: The initial capacity and the vector's growth factor should be 
 * expressed in terms of the configuration constants in vect.h
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "vect.h"

/** Main data structure for the vector. */
struct vect {
  char **data;             /* Array containing the actual data. */
  unsigned int size;       /* Number of items currently in the vector. */
  unsigned int capacity;   /* Maximum number of items the vector can hold before growing. */
};

/** Construct a new empty vector. */
vect_t *vect_new() {

  /* [TODO] Complete the function */
  
  // allocate memory for the new vector
  vect_t *v = malloc(sizeof(vect_t));

  // assign values for the vector's fields
  v->size = 0;
  v->capacity = 2;
  // allocate memeory for the data
  v->data = malloc(v->capacity * sizeof(char *));

  return v;
}

/** Delete the vector, freeing all memory it occupies. */
void vect_delete(vect_t *v) {

  /* [TODO] Complete the function */

  // free the memory allocated to each item in the data array
  for (int i = 0; i < v->size; i++) {
    free(v->data[i]);
  }
  // free the memory allocated to the vectors data
  free(v->data);
  // free the memory allocated to the vector itself
  free(v);

}

/** Get the element at the given index. */
const char *vect_get(vect_t *v, unsigned int idx) {
  assert(v != NULL);
  assert(idx < v->size);

  /* [TODO] Complete the function */

  // loop up the idx in the data array of the vector
  return v->data[idx];
}

/** Get a copy of the element at the given index. The caller is responsible
 *  for freeing the memory occupied by the copy. */
char *vect_get_copy(vect_t *v, unsigned int idx) {
  assert(v != NULL);
  assert(idx < v->size);

  /* [TODO] Complete the function */

  // create a copy of the data and allocate memory based on its length
  char *copy = malloc(strlen(v->data[idx]) + 1);

  // Copy the string from the vector to the new memory
  strcpy(copy, v->data[idx]);

  return copy;
}

/** Set the element at the given index. */
void vect_set(vect_t *v, unsigned int idx, const char *elt) {
  assert(v != NULL);
  assert(idx < v->size);

  /* [TODO] Complete the function */

  // free the memory for the old element
  free(v->data[idx]);

  //allocate memeory for the new element
  v->data[idx] = malloc(strlen(elt) + 1);

  // set the data at idx to the pointer to a new char
  strcpy(v->data[idx], elt);

}

/** Add an element to the back of the vector. */
void vect_add(vect_t *v, const char *elt) {
  assert(v != NULL);

  /* [TODO] Complete the function */

  // check if the vector needs more memeory
  if (v->size >= v->capacity) {
    // calculate the new capacity
    int new_capacity = v->capacity * 2;
    // reallocate memory to the new data
    char **new_data = realloc(v->data, new_capacity * sizeof(char *));
    // update data pointer and the vector's capacity
    v->data = new_data;
    v->capacity = new_capacity;
  }

  // allocate memory for the new char
  v->data[v->size] = malloc(strlen(elt) + 1);

  // copy the char into the allocated memory
  strcpy(v->data[v->size], elt);

  // increment the size of the vector
  v->size++;

}

/** Remove the last element from the vector. */
void vect_remove_last(vect_t *v) {
  assert(v != NULL);

  /* [TODO] Complete the function */

  // free the memory allocated to the last element in the vector
  free(v->data[v->size - 1]);

  // decrement the size
  v->size--;
}

/** The number of items currently in the vector. */
unsigned int vect_size(vect_t *v) {
  assert(v != NULL);

  /* [TODO] Complete the function */

  return v->size;
}

/** The maximum number of items the vector can hold before it has to grow. */
unsigned int vect_current_capacity(vect_t *v) {
  assert(v != NULL);

  /* [TODO] Complete the function */

  return v->capacity;
}