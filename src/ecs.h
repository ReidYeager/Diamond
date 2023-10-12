
#ifndef ECS_H
#define ECS_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "array.h"
#include "hash.h"

#pragma warning(disable:6387)
#pragma warning(disable:6386)
#pragma warning(disable:6011)

typedef uint32_t EcsId;
typedef EcsId Entity;
typedef EcsId ComponentId;
typedef EcsId ArchetypeId;

typedef struct Archetype
{
  const uint32_t componentTypeCount;
  ComponentId* pComponentTypes;
  DynamicArray* pComponentArrays;
  DynamicArray indexToEntity;

  HashMap addArchetypes; // <ComponentId, Archetype*>
  HashMap removeArchetypes; // <ComponentId, Archetype*>

  ArchetypeId id;
} Archetype;

typedef struct EntityRecord
{
  Archetype* archetype;
  uint32_t index; // Index of this entity's components in the archetype's component arrays
} EntityRecord;

typedef struct ComponentInfo
{
  uint32_t size;
  HashMap archetypeMap; // <Archetype*, uint32_t (component array index)>
} ComponentInfo;

void EcsInit();
void EcsShutdown();

ArchetypeId GetArchetypeId(uint32_t _componentCount, ComponentId* _components);
Archetype CreateArchetype(uint32_t _componentCount, ComponentId* _components);
void DestroyArchetype(Archetype* _arch);
void RemoveRowFromArchetype(Archetype* _arch, uint32_t _index);
void* MoveRowBetweenArchetypes(Archetype* _src, Archetype* _dst, uint32_t _srcIndex);
Archetype* GetArchetype(uint32_t _componentCount, ComponentId* _components);
void ComponentAddArchetype(ComponentId _component, ArchetypeId _archId, uint32_t _archComponentIndex);
Archetype* BranchArchetypeAdd(Archetype* _curArch, ComponentId _component);
Archetype* BranchArchetypeRemove(Archetype* _curArch, ComponentId _component);
void* AddComponent(Entity _entity, ComponentId _component);
void* GetComponent(Entity _entity, ComponentId _component);
void RemoveComponent(Entity _entity, ComponentId _component);
ComponentId DefineComponent(uint32_t _size);
Entity CreateEntity();

void PrintEntityComponents(Entity _entity);

#endif // !ECS_H
