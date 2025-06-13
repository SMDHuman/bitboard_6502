#ifndef P_ARRAY_H
#define P_ARRAY_H

//-----------------------------------------------------------------------------
#include <stdint.h>
#include <stddef.h>

typedef struct
{   
    size_t length;      // number of elements
    size_t size;        // capacity in length
    uint8_t type_size;  // size of each element in bytes
    void *buffer;       // pointer to the buffer holding the data
} array;

//-----------------------------------------------------------------------------

array *array_create(size_t size, size_t type_size);
void array_delete(array *arr);
void array_push(array *arr, void *data);
void array_pop(array *arr, void *data);
void array_get(array *arr, size_t index, void *data);
void array_set(array *arr, size_t index, void *data);
void array_remove(array *arr, size_t index);
void array_clear(array *arr);
void array_fill(array *arr, void *data);
void array_copy_from(array *arr, void *data, size_t size);
void array_copy_to(array *arr, void *data, size_t size);

#endif

//-----------------------------------------------------------------------------
#ifdef P_ARRAY_IMPLEMENTATION
    //-----------------------------------------------------------------------------
    // Create a new array with the specified size and type size
    array *array_create(size_t size, size_t type_size){
        array *arr = (array *)malloc(sizeof(array));
        arr->length = 0;
        arr->size = size;
        arr->type_size = type_size;
        arr->buffer = malloc(size * type_size);
        return arr;
    }
    //-----------------------------------------------------------------------------
    // Delete the array and free its memory
    void array_delete(array *arr){
        free(arr->buffer);
        free(arr);
    }
    //-----------------------------------------------------------------------------
    // Push data into the array, if there is space available
    void array_push(array *arr, void *data){
        if(arr->length >= arr->size){
            return;
        }
        memcpy((uint8_t *)arr->buffer + (arr->length * arr->type_size), data, arr->type_size);
        arr->length++;
    }
    //-----------------------------------------------------------------------------
    // Pop data from the array, if there is data available
    void array_pop(array *arr, void *data){
        if(arr->length == 0){
            return;
        }
        memcpy(data, (uint8_t *)arr->buffer + ((arr->length - 1) * arr->type_size), arr->type_size);
        arr->length--;
    }
    //-----------------------------------------------------------------------------
    // Get data from the array at the specified index, if it is within bounds
    // Note: This function does not check for out-of-bounds access
    void array_get(array *arr, size_t index, void *data){
        if(index >= arr->length){
            return;
        }
        memcpy(data, (uint8_t *)arr->buffer + (index * arr->type_size), arr->type_size);
    }
    //-----------------------------------------------------------------------------
    // Set data in the array at the specified index, if it is within bounds
    // Note: This function does not check for out-of-bounds access
    void array_set(array *arr, size_t index, void *data){
        if(index >= arr->length){
            return;
        }
        memcpy((uint8_t *)arr->buffer + (index * arr->type_size), data, arr->type_size);
    }
    //-----------------------------------------------------------------------------
    // Remove an element from the array at the specified index, if it is within bounds
    void array_remove(array *arr, size_t index){
        if(index >= arr->length){
            return;
        }
        memmove((uint8_t *)arr->buffer + (index * arr->type_size), 
                (uint8_t *)arr->buffer + ((index + 1) * arr->type_size), 
                (arr->length - index - 1) * arr->type_size);
        arr->length--;
    }
    //-----------------------------------------------------------------------------
    // Clear the array by setting all elements to zero and resetting the length
    void array_clear(array *arr){
        memset(arr->buffer, 0, arr->size * arr->type_size);
        arr->length = 0;
    }
    //-----------------------------------------------------------------------------
    // Fill the array with the specified data
    void array_fill(array *arr, void *data){
        for(size_t i = 0; i < arr->size; i++){
            memcpy((uint8_t *)arr->buffer + (i * arr->type_size), data, arr->type_size);
        }
    }
    //-----------------------------------------------------------------------------
    // Copy data from the specified pointer to the array, if the size is within bounds
    void array_copy_from(array *arr, void *data, size_t size){
        if(size > arr->size){
            return;
        }
        memcpy(arr->buffer, data, size * arr->type_size);
        arr->length = size;
    }
    //-----------------------------------------------------------------------------
    // Copy data from the array to the specified pointer, if the size is within bounds
    void array_copy_to(array *arr, void *data, size_t size){
        if(size > arr->length){
            return;
        }
        memcpy(data, arr->buffer, size * arr->type_size);
    }
    //-----------------------------------------------------------------------------
#endif
