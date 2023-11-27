#if !defined(__VECTOR_H)
#define __VECTOR_H

#include<kit/types.h>

typedef struct {
    Size size;
    Size capacity;
    Object* storage;
} Vector;

/**
 * @brief Initialize vector
 * 
 * @param vector Vector struct
 */
void vector_init(Vector* vector);

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
 * @return Result Result of the operation
 */
Result vector_resize(Vector* vector, Size newCapacity);

/**
 * @brief Get object in vector
 * 
 * @param vector Vector
 * @param index Index of object in vector
 * @param retPtr Pointer to object
 * @return Result Result of the operation
 */
Result vector_get(Vector* vector, Index64 index, Object* retPtr);

/**
 * @brief Set object in vector
 * 
 * @param vector Vector
 * @param index Index of object in vector
 * @param item Object to set
 * @return Result Result of the operation
 */
Result vector_set(Vector* vector, Index64 index, Object item);

/**
 * @brief Erase object in vector
 * 
 * @param vector Vector
 * @param index Index of object to erase
 * @return Result Result of the operation
 */
Result vector_erease(Vector* vector, Index64 index);

/**
 * @brief Get last object of vector
 * 
 * @param vector Vector
 * @param retPtr Pointer to object
 * @return Result Result of the operation
 */
Result vector_back(Vector* vector, Object* retPtr);

/**
 * @brief Push object to the end of vector
 * 
 * @param vector Vector
 * @param item Object to push
 * @return Result Result of the operation
 */
Result vector_push(Vector* vector, Object item);

/**
 * @brief Pop object from end of vector
 * 
 * @param vector Vector
 * @return Result Result of the operation
 */
Result vector_pop(Vector* vector);

#endif // __VECTOR_H
