
#ifndef ECS_B_H
#define ECS_B_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint32_t EcsId;
typedef EcsId EcsEntity;
typedef uint32_t EcsComponentMask;

#define ECS_INVALID_ENTITY 0

struct EcsWorld
{
  uint32_t entitiesCapacity;
  uint32_t entitiesCount;
  EcsEntity* pEntities;
  EcsComponentMask* pEntityMasks;
  uint32_t* entityToIndexMap;

  uint32_t availableEntitiesCapacity;
  uint32_t availableEntitiesCount;
  EcsEntity* pAvailableEntities;
};

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

  return newWorld;
}

const EcsEntity EcsCreateEntity(EcsWorld* _world)
{
  EcsEntity newEntity = 0;
  uint32_t newIndex = 0;

  if (_world->availableEntitiesCount > 0)
  {
    _world->availableEntitiesCount--;
    newEntity = _world->pAvailableEntities[_world->availableEntitiesCount];
    newIndex = _world->entitiesCount;
    _world->entitiesCount++;
    _world->pEntities[newIndex] = newEntity;
    _world->pEntityMasks[newIndex] = 0;
    _world->entityToIndexMap[newEntity] = newIndex;
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
  _world->entityToIndexMap[newEntity] = newIndex;

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

  uint32_t entityIndex = _world->entityToIndexMap[_entity];
  uint32_t entityTailIndex = _world->entitiesCount - 1;
  _world->pEntities[entityIndex] = _world->pEntities[entityTailIndex];
  _world->pEntityMasks[entityIndex] = _world->pEntityMasks[entityTailIndex];
  _world->entityToIndexMap[_world->pEntities[entityTailIndex]] = entityIndex;
  _world->entitiesCount--;
}

#endif // !ECS_B_H
