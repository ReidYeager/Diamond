
#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdint.h>
#include <stdbool.h>

typedef struct DynamicArray
{
  uint32_t elementSize;
  uint32_t capacity;
  uint32_t count;
  char* data;
} DynamicArray;

DynamicArray* DynamicArrayInit(uint32_t _elementSize, uint32_t _capacity);
void DynamicArrayShutdown(DynamicArray* _array);
void DynamicArrayResize(DynamicArray* _array, uint32_t _newCapacity);
void DynamicArrayPushBack(DynamicArray* _array, const void* _value);
void* DynamicArrayPopBack(DynamicArray* _array);

#endif // !DYNAMIC_ARRAY_H
