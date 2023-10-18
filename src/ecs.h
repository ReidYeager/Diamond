
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
void EcsStep();

// =====
// Entity
// =====

Entity CreateEntity();
void DestroyEntity(Entity _entity);

// =====
// Component
// =====

ComponentId DiamondEcsDefineComponent(const char* name, uint32_t _size);
void* DiamondEcsAddComponentById(Entity _entity, ComponentId _component);
void* DiamondEcsGetComponentById(Entity _entity, ComponentId _component);
void* DiamondEcsGetComponent(Entity _entity, const char* name);
void DiamondEcsRemoveComponent(Entity _entity, const char* name);
void DiamondEcsSetComponent(Entity entity, const char* name, const void* value);

#define Stringify(x) #x
#define StringifyDefine(x) Stringify(x)
#define EcsId(thing) Diamond_Ecs_ID_##thing

#define EcsDefineComponent(comp) DiamondEcsDefineComponent(StringifyDefine(EcsId(comp)), sizeof(comp))
#define EcsSet(entity, comp, ...)                                     \
{                                                                     \
  comp tmp = (comp)__VA_ARGS__;                                       \
  DiamondEcsSetComponent(entity, StringifyDefine(EcsId(comp)), &tmp); \
}
#define EcsSetByPointer(entity, comp, valuePointer) DiamondEcsSetComponent(entity, StringifyDefine(EcsId(comp)), valuePointer)
#define EcsGet(entity, comp) (comp*)DiamondEcsGetComponent(entity, StringifyDefine(EcsId(comp)))
#define EcsRemove(entity, comp) DiamondEcsRemoveComponent(entity, StringifyDefine(EcsId(comp)))

// =====
// Archetype
// =====

ArchetypeId GetArchetypeId(uint32_t _componentCount, ComponentId* _components);
Archetype CreateArchetype(uint32_t _componentCount, ComponentId* _components);
void DestroyArchetype(Archetype* _arch);
void* MoveRowBetweenArchetypes(Archetype* _src, Archetype* _dst, uint32_t _srcIndex);
Archetype* GetArchetype(uint32_t _componentCount, ComponentId* _components);
void ComponentAddArchetype(ComponentId _component, ArchetypeId _archId, uint32_t _archComponentIndex);
Archetype* BranchArchetypeAdd(Archetype* _curArch, ComponentId _component);
Archetype* BranchArchetypeRemove(Archetype* _curArch, ComponentId _component);
bool ArchetypeHasComponent(Archetype* arch, ComponentId component);

// =====
// System
// =====

typedef struct SystemIterator
{
  uint32_t entityCount;
  uint32_t entityIndex;
  Entity* entities; // All entities whose components will be iterated over

  uint32_t archetypeCount;
  Archetype* archetypes;

  uint32_t componetCount;
  ComponentId* componentIds;
} SystemIterator;

typedef void(*EcsSystemFunction)(SystemIterator* iterator);

typedef struct System
{
  uint32_t componetCount;
  ComponentId* componentIds;
  EcsSystemFunction function;
} System;

void DiamondEcsDefineSystem(EcsSystemFunction function, const char* componentNames);
void* DiamondEcsGetSystemComponent(SystemIterator* iterator, const char* componentName);

#define EcsDefineSystem(function, ...) DiamondEcsDefineSystem(function, #__VA_ARGS__)
#define EcsGetSystemComponent(iterator, comp) (comp*)DiamondEcsGetSystemComponent(iterator, StringifyDefine(EcsId(comp)));

// =====
// Debug
// =====

void PrintEntityComponents(Entity _entity);
void PrintComponentArchetypes(ComponentId _component);
void PrintAllArchetypeComponents();
void PrintAllArchetypeEntities();


#endif // !ECS_H
