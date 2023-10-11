
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

void HashMapShutdown(HashMap* _map)
{
  for (uint32_t i = 0; i < _map->bucketCount; i++)
  {
    for (HashElement* e = _map->pBuckets[i]; e != NULL;)
    {
      HashElement* next = e->next;
      free(e->value);
      e = next;
    }
  }
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

void HashMapRemove(HashMap* _map, uint32_t _key)
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

      free(e->value);
      free(e);
      break;
    }

    prev = e;
    e = e->next;
  }
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
