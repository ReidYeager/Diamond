
#ifndef ECS_B_H
#define ECS_B_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t EcsId;
typedef EcsId EcsEntity;
typedef uint32_t EcsComponentMask;

#define ECS_INVALID_ENTITY 0
#define ECS_COMPONENT_CAPACITY 32

#define __ECS_ENTITY_MASK_INDEX 0x00000000ffffffff
#define __ECS_ENTITY_MASK_ITERATION 0xffffffff00000000

#define __EcsEntityIndex(entity) (entity & __ECS_ENTITY_MASK_INDEX)

EcsEntity __EcsEntityIncrementIteration(EcsEntity _entity)
{
  uint64_t iteration = (_entity >> 32) + 1;
  return (_entity & __ECS_ENTITY_MASK_INDEX) | (iteration << 32);
}

typedef struct EcsComponentSet
{
  uint32_t id;
  const char* name;
  uint32_t size;

  uint32_t count;
  uint32_t capacity;
  char* data;

  uint32_t entityMapCapacity;
  uint32_t* entityToDataIndexMap;
} EcsComponentSet;

typedef struct EcsWorld
{
  uint32_t entitiesCapacity;
  uint32_t entitiesCount;
  EcsEntity* pEntities;
  EcsComponentMask* pEntityMasks;
  uint32_t* entityToIndexMap;

  uint32_t availableEntitiesCapacity;
  uint32_t availableEntitiesCount;
  EcsEntity* pAvailableEntities;

  uint32_t componentCount;
  EcsComponentSet* pComponentSets;
} EcsWorld;

EcsWorld* EcsInit()
{
  EcsWorld* newWorld = (EcsWorld*)malloc(sizeof(EcsWorld));
  memset(newWorld, 0, sizeof(EcsWorld));

  newWorld->entitiesCapacity = 2;
  newWorld->entitiesCount = 0;
  newWorld->pEntities = (EcsEntity*)malloc(sizeof(EcsEntity) * newWorld->entitiesCapacity);
  newWorld->pEntityMasks = (EcsComponentMask*)malloc(sizeof(EcsComponentMask) * newWorld->entitiesCapacity);
  newWorld->entityToIndexMap = (uint32_t*)malloc(sizeof(uint32_t) * newWorld->entitiesCapacity);

  newWorld->availableEntitiesCapacity = 2;
  newWorld->availableEntitiesCount = 0;
  newWorld->pAvailableEntities = (EcsEntity*)malloc(sizeof(EcsEntity) * newWorld->availableEntitiesCapacity);

  newWorld->componentCount = 0;
  newWorld->pComponentSets = (EcsComponentSet*)malloc(sizeof(EcsComponentSet) * ECS_COMPONENT_CAPACITY);

  return newWorld;
}

const EcsEntity EcsCreateEntity(EcsWorld* _world)
{
  EcsEntity newEntity = 0;
  uint32_t newIndex = 0;

  // TODO : Find a different way to do entity Id's so they don't get re-used as easily
  // Being re-used can quickly cause issues when an entity is destroyed, another takes its place
  // but a reference to the original is later used by the user
  if (_world->availableEntitiesCount > 0)
  {
    _world->availableEntitiesCount--;
    newEntity = _world->pAvailableEntities[_world->availableEntitiesCount];
    newEntity = __EcsEntityIncrementIteration(newEntity);
    newIndex = _world->entitiesCount;
    _world->entitiesCount++;
    _world->pEntities[newIndex] = newEntity;
    _world->pEntityMasks[newIndex] = 0;
    _world->entityToIndexMap[__EcsEntityIndex(newEntity)] = newIndex;
    return _world->pEntities[newIndex];
  }

  if (_world->entitiesCount == _world->entitiesCapacity)
  {
    _world->entitiesCapacity *= 2;

    EcsEntity* newEntities = (EcsEntity*)malloc(sizeof(EcsEntity) * _world->entitiesCapacity);
    memcpy(newEntities, _world->pEntities, sizeof(EcsEntity) * _world->entitiesCount);
    free(_world->pEntities);
    _world->pEntities = newEntities;

    EcsComponentMask* newMasks = (EcsComponentMask*)malloc(sizeof(EcsComponentMask) * _world->entitiesCapacity);
    memcpy(newMasks, _world->pEntityMasks, sizeof(EcsComponentMask) * _world->entitiesCount);
    free(_world->pEntityMasks);
    _world->pEntityMasks = newMasks;

    uint32_t* newMap = (uint32_t*)malloc(sizeof(uint32_t) * _world->entitiesCapacity);
    memcpy(newMap, _world->entityToIndexMap, sizeof(uint32_t) * _world->entitiesCount);
    free(_world->entityToIndexMap);
    _world->entityToIndexMap = newMap;
  }

  newEntity = _world->entitiesCount;
  newIndex = _world->entitiesCount;
  _world->entitiesCount++;
  _world->pEntities[newIndex] = newEntity;
  _world->pEntityMasks[newIndex] = 0;
  _world->entityToIndexMap[__EcsEntityIndex(newEntity)] = newIndex;

  return newEntity;
}

void EcsDestroyEntity(EcsWorld* _world, EcsEntity _entity)
{
  // TODO : Validate that the entity is even valid to begin with
  if (_world->availableEntitiesCount == _world->availableEntitiesCapacity)
  {
    _world->availableEntitiesCapacity *= 2;
    EcsEntity* newAvailable = (EcsEntity*)malloc(sizeof(EcsEntity) * _world->availableEntitiesCapacity);
    memcpy(newAvailable, _world->pAvailableEntities, sizeof(EcsEntity) * _world->availableEntitiesCount);
    free(_world->pAvailableEntities);
    _world->pAvailableEntities = newAvailable;
  }

  _world->pAvailableEntities[_world->availableEntitiesCount] = _entity;
  _world->availableEntitiesCount++;

  uint32_t entityIndex = _world->entityToIndexMap[__EcsEntityIndex(_entity)];

  for (uint32_t i = 0; i < _world->componentCount; i++)
  {
    if (_world->pEntityMasks[entityIndex] & (1 << i))
    {
      EcsComponentSet* set = &_world->pComponentSets[i];
      memcpy(&set->data[set->entityToDataIndexMap[__EcsEntityIndex(_entity)]], &set->data[set->entityToDataIndexMap[set->count - 1]], set->size);
      set->entityToDataIndexMap[set->count - 1] = set->entityToDataIndexMap[__EcsEntityIndex(_entity)];
      set->count--;
    }
  }

  uint32_t entityTailIndex = _world->entitiesCount - 1;
  _world->pEntities[entityIndex] = _world->pEntities[entityTailIndex];
  _world->pEntityMasks[entityIndex] = _world->pEntityMasks[entityTailIndex];
  _world->entityToIndexMap[_world->pEntities[entityTailIndex]] = entityIndex;
  _world->entitiesCount--;

}

void __EcsDefineComponent(EcsWorld* _world, const char* _name, uint32_t _size)
{
  for (uint32_t i = 0; i < _world->componentCount; i++)
  {
    if (!strcmp(_name, _world->pComponentSets[i].name))
    {
      return;
    }
  }

  if (_world->componentCount == 32)
  {
    // TODO : Log component max hit error
    return;
  }

  EcsComponentSet newSet = { 0 };
  newSet.id = _world->componentCount;
  newSet.name = _name;
  newSet.size = _size;
  newSet.capacity = 8;
  newSet.count = 0;
  newSet.data = (char*)malloc(_size * newSet.capacity);
  newSet.entityMapCapacity = newSet.capacity;
  newSet.entityToDataIndexMap = (uint32_t*)malloc(sizeof(uint32_t) * newSet.entityMapCapacity);

  _world->pComponentSets[newSet.id] = newSet;
  _world->componentCount++;
}

#define EcsDefineComponent(world, component) __EcsDefineComponent(world, #component, sizeof(component));

void __EcsSet(EcsWorld* world, EcsEntity entity, const char* component, const void* value)
{
  uint32_t componentIndex = ~0u;

  for (uint32_t i = 0; i < world->componentCount; i++)
  {
    if (!strcmp(component, world->pComponentSets[i].name))
    {
      componentIndex = i;
      break;
    }
  }

  if (componentIndex == ~0u)
    return;

  EcsComponentSet* set = &world->pComponentSets[componentIndex];

  if (set->count == set->capacity)
  {
    set->capacity *= 2;
    char* newData = (char*)malloc(set->size * set->capacity);
    memcpy(newData, set->data, set->size * set->count);
    free(set->data);
    set->data = newData;
  }

  if (__EcsEntityIndex(entity) > set->entityMapCapacity)
  {
    uint32_t oldCapacity = set->entityMapCapacity;

    while (__EcsEntityIndex(entity) > set->entityMapCapacity)
    {
      set->entityMapCapacity *= 2;
    }

    uint32_t* newIndexMap = (uint32_t*)malloc(sizeof(uint32_t) * set->entityMapCapacity);
    memcpy(newIndexMap, set->entityToDataIndexMap, sizeof(uint32_t) * oldCapacity);
    free(set->entityToDataIndexMap);
    set->entityToDataIndexMap = newIndexMap;
  }

  memcpy(set->data + (set->size * set->count), value, set->size);
  set->entityToDataIndexMap[__EcsEntityIndex(entity)] = set->size * set->count;
  set->count++;

  world->pEntityMasks[world->entityToIndexMap[__EcsEntityIndex(entity)]] |= (1 << set->id);
}

#define EcsSetPointer(world, entity, component, valuePtr) __EcsSet(world, entity, #component, valuePtr);
#define EcsSet(world, entity, component, ...) __EcsSet(world, entity, #component, &(component)__VA_ARGS__);

void* __EcsGet(EcsWorld* world, EcsEntity entity, const char* component)
{
  EcsComponentSet* set = NULL;

  int entityFound = 0;
  for (uint32_t i = 0; i < world->entitiesCount; i++)
  {
    if (world->pEntities[i] == entity)
    {
      entityFound = 1;
      break;
    }
  }
  if (!entityFound)
    return NULL;

  for (uint32_t i = 0; i < world->componentCount; i++)
  {
    if (!strcmp(component, world->pComponentSets[i].name))
    {
      set = &world->pComponentSets[i];
      break;
    }
  }

  if (set == NULL)
    return NULL;

  uint32_t index = set->entityToDataIndexMap[__EcsEntityIndex(entity)];
  return &set->data[index];
}

#define EcsGet(world, entity, component) (component*)__EcsGet(world, entity, #component);

#endif // !ECS_B_H
