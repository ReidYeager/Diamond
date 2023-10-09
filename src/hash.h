
#ifndef ECS_HASH_MAP_H
#define ECS_HASH_MAP_H

#include <stdbool.h>
#include <stdint.h>

typedef struct HashElement
{
  uint32_t value;
  uint32_t key;
  uint32_t keyHash;

  struct HashElement* next;
} HashElement;

typedef struct HashMap
{
  uint32_t elementCount;

  // TODO : Expand buckets list when any one reaches some max chain limit
  uint32_t bucketCount;
  HashElement** pBuckets;
} HashMap;

HashMap* HashMapInit();
void HashMapShutdown(HashMap* _map);
void HashMapInsert(HashMap* _map, uint32_t _key, uint32_t _value);
void HashMapRemove(HashMap* _map, uint32_t _key);
uint32_t* HashMapGet(HashMap* _map, uint32_t _key);

#endif // !ECS_HASH_MAP_H
