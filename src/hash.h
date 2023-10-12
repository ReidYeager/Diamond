
#ifndef ECS_HASH_MAP_H
#define ECS_HASH_MAP_H

#include <stdbool.h>
#include <stdint.h>

typedef struct HashElement
{
  void* value;
  uint32_t key;
  uint32_t keyHash;

  struct HashElement* next;
} HashElement;

typedef struct HashMap
{
  uint32_t elementCount;
  uint32_t elementSize;

  // TODO : Expand buckets list when any one reaches some max chain limit
  uint32_t bucketCount;
  HashElement** pBuckets;
} HashMap;

typedef void(*HashMapValueShutdownFunction)(void*);

HashMap HashMapInit(uint32_t _elementSize);
void HashMapShutdown(HashMap* _map, HashMapValueShutdownFunction _valueShutdownFunction);
void* HashMapSet(HashMap* _map, uint32_t _key, void* _value);
void HashMapRemove(HashMap* _map, uint32_t _key, HashMapValueShutdownFunction _valueShutdownFunction);
void* HashMapGet(HashMap* _map, uint32_t _key);
#define HashMapGetAs(map, key, type) (type*)HashMapGet(map, key)

#endif // !ECS_HASH_MAP_H
