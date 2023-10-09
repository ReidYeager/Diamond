
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "array.h"

DynamicArray* DynamicArrayInit(uint32_t _elementSize, uint32_t _capacity)
{
  assert(_capacity);
  DynamicArray* newArray = (DynamicArray*)malloc(sizeof(DynamicArray));
  assert(newArray);

  newArray->capacity = _capacity;
  newArray->count = 0;
  newArray->elementSize = _elementSize;
  newArray->data = (char*)malloc(_elementSize * _capacity);
  assert(newArray->data);

  return newArray;
}

void DynamicArrayShutdown(DynamicArray* _array)
{
  free(_array->data);
  free(_array);
}

void DynamicArrayResize(DynamicArray* _array, uint32_t _newCapacity)
{
  assert(_newCapacity);
  char* newData = (char*)realloc(_array->data, _array->elementSize * _newCapacity);
  assert(newData);

  _array->data = newData;
  _array->capacity = _newCapacity;

  if (_array->count > _newCapacity)
    _array->count = _newCapacity;
}

void DynamicArrayPushBack(DynamicArray* _array, const void* _value)
{
  if (_array->count >= _array->capacity)
  {
    DynamicArrayResize(_array, _array->capacity * 2);
  }

  memcpy(&_array->data[_array->elementSize * _array->count], _value, _array->elementSize);
  _array->count++;
}

void* DynamicArrayPopBack(DynamicArray* _array)
{
  if (_array->count == 0)
    return NULL;

  _array->count--;
  return &_array->data[_array->elementSize * _array->count];
}
