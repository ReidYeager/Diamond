
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "array.h"

DynamicArray DynamicArrayInit(uint32_t _elementSize, uint32_t _capacity)
{
  assert(_capacity);
  DynamicArray newArray = { 0 };

  newArray.capacity = _capacity;
  newArray.count = 0;
  newArray.elementSize = _elementSize;
  newArray.data = (char*)malloc(_elementSize * _capacity);
  assert(newArray.data);

  return newArray;
}

void DynamicArrayShutdown(DynamicArray* _array)
{
  free(_array->data);
}

void DynamicArrayResize(DynamicArray* _array, uint32_t _newCapacity)
{
  assert(_newCapacity);
  void* newData = (void*)realloc(_array->data, _array->elementSize * _newCapacity);
  assert(newData);

  _array->data = newData;
  _array->capacity = _newCapacity;

  if (_array->count > _newCapacity)
    _array->count = _newCapacity;
}

void* DynamicArrayPushBack(DynamicArray* _array, const void* _value)
{
  if (_array->count >= _array->capacity)
  {
    DynamicArrayResize(_array, _array->capacity * 2);
  }

  void* valuePtr = (char*)_array->data + (_array->elementSize * _array->count);
  memcpy(valuePtr, _value, _array->elementSize);
  _array->count++;

  return valuePtr;
}

void* DynamicArrayPushEmpty(DynamicArray* _array)
{
  if (_array->count >= _array->capacity)
  {
    DynamicArrayResize(_array, _array->capacity * 2);
  }

  void* valuePtr = (char*)_array->data + (_array->elementSize * _array->count);
  memset(valuePtr, 0, _array->elementSize);
  _array->count++;

  return valuePtr;
}

void* DynamicArrayPopBack(DynamicArray* _array)
{
  if (_array->count == 0)
    return NULL;

  _array->count--;
  return (char*)_array->data + (_array->elementSize * _array->count);
}

void* DynamicArrayGet(DynamicArray* _array, uint32_t _index)
{
  return (char*)_array->data + (_index * _array->elementSize);
}

void DynamicArraySet(DynamicArray* _array, uint32_t _index, void* _value)
{
  memcpy((char*)_array->data + (_index * _array->elementSize), _value, _array->elementSize);
}
