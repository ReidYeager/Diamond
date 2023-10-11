
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

HashMap archetypes; // <ArchetypeId, Archetype>
HashMap entityRecords; // <Entity, EntityRecord>
HashMap componentInfos; // <componentId, ComponentInfo>

ArchetypeId GetArchetypeId(uint32_t _componentCount, ComponentId* _components)
{
  ArchetypeId newId = 0;

  char* pComponentsBytes = (char*)_components;
  for (uint32_t i = 0; i < _componentCount * sizeof(ComponentId); i++)
  {
    newId = 31 * newId + pComponentsBytes[i];
  }

  return newId;
}

Archetype CreateArchetype(uint32_t _componentCount, ComponentId* _components)
{
  Archetype newArch = { _componentCount };
  newArch.id = GetArchetypeId(_componentCount, _components);

  if (_componentCount)
  {
    newArch.pComponentTypes = (ComponentId*)malloc(sizeof(ComponentId) * _componentCount);
    assert(newArch.pComponentTypes);
    memcpy(newArch.pComponentTypes, _components, sizeof(ComponentId) * _componentCount);

    newArch.pComponentArrays = (DynamicArray*)malloc(sizeof(DynamicArray) * _componentCount);
    assert(newArch.pComponentArrays);

    for (uint32_t i = 0; i < newArch.componentTypeCount; i++)
    {
      ComponentInfo* cInfo = (ComponentInfo*)HashMapGet(&componentInfos, _components[i]);
      newArch.pComponentArrays[i] = DynamicArrayInit(cInfo->size, 2);
    }
  }

  newArch.addArchetypes = HashMapInit(sizeof(Archetype*));
  newArch.removeArchetypes= HashMapInit(sizeof(Archetype*));

  return newArch;
}

Archetype* GetArchetype(uint32_t _componentCount, ComponentId* _components)
{
  uint32_t id = GetArchetypeId(_componentCount, _components);
  return (Archetype*)HashMapGet(&archetypes, id);
}

void ComponentAddArchetype(ComponentId _component, ArchetypeId _archId, uint32_t _archComponentIndex)
{
  ComponentInfo* cinfo = (ComponentInfo*)HashMapGet(&componentInfos, _component);
  HashMapSet(&cinfo->archetypeMap, _archId, &_archComponentIndex);
}

// TODO : Create version that removes the input component
// Creates a new archetype with components for all in _curArch with _component added to it
Archetype* BranchArchetypeAdd(Archetype* _curArch, ComponentId _component)
{
  // TODO : Make sure the archetype doesn't already have this component

  uint32_t count = _curArch->componentTypeCount + 1;
  ComponentId* newTypes = (ComponentId*)malloc(sizeof(ComponentId) * count);

  // Build new component list and maintain sorting
  ComponentId* oldTypes = _curArch->pComponentTypes;
  ComponentId heldId = _component;
  for (uint32_t i = 0; i < count - 1; i++)
  {
    if (oldTypes[i] < heldId)
    {
      newTypes[i] = oldTypes[i];
    }
    else
    {
      newTypes[i] = heldId;
      heldId = oldTypes[i];
    }
  }
  newTypes[count - 1] = heldId;

  // Test if this archetype already exists
  ArchetypeId archId = GetArchetypeId(count, newTypes);
  Archetype* newArchPtr = (Archetype*)HashMapGet(&archetypes, archId);

  if (newArchPtr == NULL)
  {
    Archetype newArch = CreateArchetype(count, newTypes);
    newArchPtr = (Archetype*)HashMapSet(&archetypes, newArch.id, &newArch);
  }

  HashMapSet(&_curArch->addArchetypes, newArchPtr->id, newArchPtr);
  HashMapSet(&newArchPtr->removeArchetypes, _curArch->id, _curArch);

  for (uint32_t i = 0; i < count; i++)
  {
    ComponentAddArchetype(newArchPtr->pComponentTypes[i], newArchPtr->id, i);
  }

  return newArchPtr;
}

void* AddComponent(Entity _entity, ComponentId _component)
{
  // TODO : Make sure the entity doesn't already have the component

  EntityRecord* record = (EntityRecord*)HashMapGet(&entityRecords, _entity);
  Archetype* curArch = record->archetype;
  Archetype* nextArch = (Archetype*)HashMapGet(&curArch->addArchetypes, _component);

  if (nextArch == NULL)
  {
    nextArch = BranchArchetypeAdd(curArch, _component);
  }

  void* valuePtr = NULL;

  for (uint32_t i = 0, j = 0; i < nextArch->componentTypeCount; i++)
  {
    // expand the array
    if (j < curArch->componentTypeCount && curArch->pComponentTypes[j] == nextArch->pComponentTypes[i])
    {
      // Copy the local value to the new array
      void* curValue = DynamicArrayGet(&curArch->pComponentArrays[j], record->index);
      DynamicArrayPushBack(&nextArch->pComponentArrays[i], curValue);
      j++;
    }
    else
    {
      // Init the new value
      valuePtr = DynamicArrayPushEmpty(&nextArch->pComponentArrays[i]);
    }
  }
  record->archetype = nextArch;
  record->index = nextArch->pComponentArrays[0].count - 1;
  return valuePtr;
}

void* GetComponent(Entity _entity, ComponentId _component)
{
  void* value = NULL;

  EntityRecord* record = (EntityRecord*)HashMapGet(&entityRecords, _entity);
  Archetype* arch = record->archetype;

  ComponentInfo* cinfo = (ComponentInfo*)HashMapGet(&componentInfos, _component);
  uint32_t* vptr = (uint32_t*)HashMapGet(&cinfo->archetypeMap, arch->id);
  assert(vptr);
  uint32_t componentIndex = *vptr;

  return DynamicArrayGet(&arch->pComponentArrays[componentIndex], record->index);
}

// TODO : Define a component default value?
ComponentId DefineComponent(uint32_t _size)
{
  static uint32_t componentCount = 0;
  componentCount++;

  ComponentInfo cinfo = { 0 };
  cinfo.size = _size;
  cinfo.archetypeMap = HashMapInit(sizeof(uint32_t));

  HashMapSet(&componentInfos, componentCount, &cinfo);

  return componentCount;
}

Entity CreateEntity()
{
  static uint32_t entityCount = 0;
  entityCount++;

  EntityRecord newRecord = { 0 };
  newRecord.archetype = HashMapGet(&archetypes, 0); // Default empty
  newRecord.index = 0; // Doesn't matter w/ the empty archetype

  HashMapSet(&entityRecords, entityCount, &newRecord);
  return entityCount;
}

typedef struct foo
{
  float x, y, z;
} foo;

int main()
{
  archetypes = HashMapInit(sizeof(Archetype));
  entityRecords = HashMapInit(sizeof(EntityRecord));
  componentInfos = HashMapInit(sizeof(ComponentInfo));

  Archetype emptyArch = CreateArchetype(0, NULL);
  HashMapSet(&archetypes, 0, &emptyArch);

  ComponentId intComponent = DefineComponent(sizeof(uint32_t));
  ComponentId fooComponent = DefineComponent(sizeof(foo));

  Entity e = CreateEntity();

  uint32_t* v = (uint32_t*)AddComponent(e, intComponent);

  *v = 1234;

  printf("New value set to : %u\n", *(uint32_t*)GetComponent(e, intComponent));

  foo* f = (foo*)AddComponent(e, fooComponent);
  f->x = 10.0f;
  f->y = 12.55f;
  f->z = 1002.558f;

  printf("tmp : %u --- foo: %f, %f, %f\n",
    *(uint32_t*)GetComponent(e, intComponent),
    ((foo*)GetComponent(e, fooComponent))->x,
    ((foo*)GetComponent(e, fooComponent))->y,
    ((foo*)GetComponent(e, fooComponent))->z);


  Entity e2 = CreateEntity();

  f = (foo*)AddComponent(e2, fooComponent);
  f->x = 20.0f;
  f->y = 22.55f;
  f->z = 2002.558f;

  v = (uint32_t*)AddComponent(e2, intComponent);

  *v = 2234;

  printf("tmp : %u --- foo: %f, %f, %f\n",
    *(uint32_t*)GetComponent(e2, intComponent),
    ((foo*)GetComponent(e2, fooComponent))->x,
    ((foo*)GetComponent(e2, fooComponent))->y,
    ((foo*)GetComponent(e2, fooComponent))->z);

  printf("\n");

  ComponentId syscomponents[2] = { intComponent, fooComponent };
  Archetype* arch = GetArchetype(2, syscomponents);
  uint32_t count = arch->pComponentArrays[0].count;
  for (uint32_t i = 0; i < count; i++)
  {
    printf("tmp : %u --- foo: %f, %f, %f\n",
      *(uint32_t*)DynamicArrayGet(&arch->pComponentArrays[0], i),
      ((foo*)DynamicArrayGet(&arch->pComponentArrays[1], i))->x,
      ((foo*)DynamicArrayGet(&arch->pComponentArrays[1], i))->y,
      ((foo*)DynamicArrayGet(&arch->pComponentArrays[1], i))->z);
  }

  return 0;
}
