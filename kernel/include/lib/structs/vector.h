#if !defined(__VECTOR_H)
#define __VECTOR_H

#include<kit/types.h>

typedef struct {
    size_t size;
    size_t capacity;
    Object* storage;
} Vector;

/**
 * @brief Initialize vector
 * 
 * @param vector Vector struct
 */
void initVector(Vector* vector);

/**
 * @brief Release vector
 * 
 * @param vector vector initialized
 */
void releaseVector(Vector* vector);

/**
 * @brief Is vector empty?
 * 
 * @param vector Vector
 * @return bool True if vector is empty
 */
bool vectorIsEmpty(Vector* vector);

/**
 * @brief Clear the vector
 * 
 * @param vector Vector
 */
void vectorClear(Vector* vector);

/**
 * @brief Resize the capacity of vector
 * 
 * @param vector Vector
 * @param newCapacity New capacity to resize to, if capacity less than curretn size, data at the end will lost
 * @return Result Result of the operation
 */
Result vectorResize(Vector* vector, size_t newCapacity);

/**
 * @brief Get object in vector
 * 
 * @param vector Vector
 * @param index Index of object in vector
 * @param retPtr Pointer to object
 * @return Result Result of the operation
 */
Result vectorGet(Vector* vector, Index64 index, Object* retPtr);

/**
 * @brief Set object in vector
 * 
 * @param vector Vector
 * @param index Index of object in vector
 * @param item Object to set
 * @return Result Result of the operation
 */
Result vectorSet(Vector* vector, Index64 index, Object item);

/**
 * @brief Erase object in vector
 * 
 * @param vector Vector
 * @param index Index of object to erase
 * @return Result Result of the operation
 */
Result vectorErase(Vector* vector, Index64 index);

/**
 * @brief Get last object of vector
 * 
 * @param vector Vector
 * @param retPtr Pointer to object
 * @return Result Result of the operation
 */
Result vectorBack(Vector* vector, Object* retPtr);

/**
 * @brief Push object to the end of vector
 * 
 * @param vector Vector
 * @param item Object to push
 * @return Result Result of the operation
 */
Result vectorPush(Vector* vector, Object item);

/**
 * @brief Pop object from end of vector
 * 
 * @param vector Vector
 * @return Result Result of the operation
 */
Result vectorPop(Vector* vector);

#endif // __VECTOR_H
