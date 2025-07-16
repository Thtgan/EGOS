#if !defined(__LIB_STRUCTS_VECTOR_H)
#define __LIB_STRUCTS_VECTOR_H

typedef struct Vector Vector;

#include<kit/types.h>

typedef struct Vector {
    Size size;
    Size capacity;
    Object* storage;
} Vector;

/**
 * @brief Initialize vector
 * 
 * @param vector Vector struct
 */
void vector_initStruct(Vector* vector);

void vector_initStructN(Vector* vector, Size n);

/**
 * @brief Release vector
 * 
 * @param vector vector initialized
 */
void vector_clearStruct(Vector* vector);

/**
 * @brief Is vector empty?
 * 
 * @param vector Vector
 * @return bool True if vector is empty
 */
bool vector_isEmpty(Vector* vector);

/**
 * @brief Clear the vector
 * 
 * @param vector Vector
 */
void vector_clear(Vector* vector);

/**
 * @brief Resize the capacity of vector
 * 
 * @param vector Vector
 * @param newCapacity New capacity to resize to, if capacity less than curretn size, data at the end will lost
 */
void vector_resize(Vector* vector, Size newCapacity);

/**
 * @brief Get object in vector
 * 
 * @param vector Vector
 * @param index Index of object in vector
 * @param retPtr Pointer to object
 */
Object vector_get(Vector* vector, Index64 index);

/**
 * @brief Set object in vector
 * 
 * @param vector Vector
 * @param index Index of object in vector
 * @param item Object to set
 */
void vector_set(Vector* vector, Index64 index, Object item);

/**
 * @brief Erase object in vector
 * 
 * @param vector Vector
 * @param index Index of object to erase
 */
void vector_erease(Vector* vector, Index64 index);

void vector_ereaseN(Vector* vector, Index64 index, Size n);

/**
 * @brief Get last object of vector
 * 
 * @param vector Vector
 * @param retPtr Pointer to object
 */
Object vector_back(Vector* vector);

/**
 * @brief Push object to the end of vector
 * 
 * @param vector Vector
 * @param item Object to push
 */
void vector_push(Vector* vector, Object item);

/**
 * @brief Pop object from end of vector
 * 
 * @param vector Vector
 */
void vector_pop(Vector* vector);

#endif // __LIB_STRUCTS_VECTOR_H
