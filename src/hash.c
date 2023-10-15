
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "hash.h"

#define HASH_MAP_DEFAULT_BUCKET_COUNT 16

uint32_t HashUint(uint32_t _key)
{
  uint32_t hash = 0;

  for (uint32_t i = 0; i < 4; i++)
  {
    hash = 31 * hash + ((char*)&_key)[i];
  }

  return hash;
}

uint32_t HashString(const char* _key)
{
  uint32_t hash = 0;

  for (uint32_t i = 0; _key[i] != '\0'; i++)
  {
    hash = 31 * hash + _key[i];
  }

  return hash;
}

HashMap HashMapInit(uint32_t _elementSize)
{
  assert(_elementSize);

  HashMap newMap = { 0 };

  newMap.elementCount = 0;
  newMap.elementSize = _elementSize;
  newMap.bucketCount = HASH_MAP_DEFAULT_BUCKET_COUNT;
  newMap.pBuckets = (HashElement**)malloc(sizeof(void*) * newMap.bucketCount);
  assert(newMap.pBuckets);

  memset(newMap.pBuckets, 0, sizeof(void*) * newMap.bucketCount);

  return newMap;
}

void HashMapShutdown(HashMap* _map, HashMapValueShutdownFunction _valueShutdownFunction)
{
  HashElement* curElement = NULL;
  HashElement* nextElement = NULL;

  for (uint32_t i = 0; i < _map->bucketCount; i++)
  {
    curElement = _map->pBuckets[i];

    while (curElement != NULL)
    {
      nextElement = curElement->next;

      if (_valueShutdownFunction)
        _valueShutdownFunction(curElement->value);
      free(curElement->value);
      free(curElement);

      curElement = nextElement;
      _map->elementCount--;
    }
  }

  free(_map->pBuckets);
  _map->bucketCount = 0;
}

void* HashMapSet(HashMap* _map, uint32_t _key, void* _value)
{
  uint32_t hashKey = HashUint(_key);

  HashElement* newElement = (HashElement*)malloc(sizeof(HashElement));
  assert(newElement);

  uint32_t bucketIndex = hashKey % _map->bucketCount;

  newElement->key = _key;
  newElement->keyHash = hashKey;
  newElement->next = _map->pBuckets[bucketIndex];

  newElement->value = malloc(_map->elementSize);
  assert(newElement->value);
  memcpy(newElement->value, _value, _map->elementSize);

  _map->pBuckets[bucketIndex] = newElement;
  _map->elementCount++;

  return newElement->value;
}

void* HashMapStringSet(HashMap* _map, const char* _key, void* _value)
{
  uint32_t key = HashString(_key);
  return HashMapSet(_map, key, _value);
}

void HashMapRemove(HashMap* _map, uint32_t _key, HashMapValueShutdownFunction _valueShutdownFunction)
{
  uint32_t hashKey = HashUint(_key);
  uint32_t bucketIndex = hashKey % _map->bucketCount;

  HashElement* prev = NULL;
  HashElement* e = _map->pBuckets[bucketIndex];
  while (e != NULL)
  {
    if (e->key == _key)
    {
      if (prev)
        prev->next = e->next;
      else
        _map->pBuckets[bucketIndex] = e->next;

      if (_valueShutdownFunction)
        _valueShutdownFunction(e->value);
      free(e->value);
      free(e);
      break;
    }

    prev = e;
    e = e->next;
  }
}

void HashMapStringRemove(HashMap* _map, const char* _key, HashMapValueShutdownFunction _valueShutdownFunction)
{
  uint32_t key = HashString(_key);
  HashMapRemove(_map, key, _valueShutdownFunction);
}

void* HashMapGet(HashMap* _map, uint32_t _key)
{
  uint32_t hashKey = HashUint(_key);
  uint32_t bucketIndex = hashKey % _map->bucketCount;

  HashElement* e = _map->pBuckets[bucketIndex];
  while (e != NULL)
  {
    if (e->key == _key)
    {
      return e->value;
    }

    e = e->next;
  }

  return NULL;
}

void* HashMapStringGet(HashMap* _map, const char* _key)
{
  uint32_t key = HashString(_key);
  return HashMapGet(_map, key);
}
