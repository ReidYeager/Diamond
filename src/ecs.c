
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "array.h"
#include "hash.h"
#include "ecs.h"

HashMap archetypes; // <ArchetypeId, Archetype>
HashMap entityRecords; // <Entity, EntityRecord>
HashMap componentInfos; // <componentId, ComponentInfo>
HashMap ComponentTypeToId; // <string, ComponentId>
uint32_t entityCount = 0;

DynamicArray systems; // System

void EcsInit()
{
  archetypes = HashMapInit(sizeof(Archetype));
  entityRecords = HashMapInit(sizeof(EntityRecord));
  componentInfos = HashMapInit(sizeof(ComponentInfo));
  ComponentTypeToId = HashMapInit(sizeof(ComponentId));

  systems = DynamicArrayInit(sizeof(System), 2);

  Archetype emptyArch = CreateArchetype(0, NULL);
  HashMapSet(&archetypes, 0, &emptyArch);
}

void EcsShutdown()
{
  for (uint32_t i = 0; i < systems.count; i++)
  {
    System* sys = (System*)DynamicArrayGet(&systems, i);
    free(sys->componentIds);
  }
  DynamicArrayShutdown(&systems);

  HashMapShutdown(&archetypes, (HashMapValueShutdownFunction)DestroyArchetype);
  HashMapShutdown(&entityRecords, NULL);
  HashMapShutdown(&componentInfos, NULL);
}

void DestroyArchetype(Archetype* _arch)
{
  for (uint32_t i = 0; i < _arch->componentTypeCount; i++)
  {
    DynamicArrayShutdown(&_arch->pComponentArrays[i]);
  }

  HashMapShutdown(&_arch->addArchetypes, NULL);
  HashMapShutdown(&_arch->removeArchetypes, NULL);

  DynamicArrayShutdown(&_arch->indexToEntity);

  free(_arch->pComponentArrays);
  free(_arch->pComponentTypes);
}

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

  newArch.indexToEntity = DynamicArrayInit(sizeof(Entity), 4);

  newArch.addArchetypes = HashMapInit(sizeof(Archetype*));
  newArch.removeArchetypes = HashMapInit(sizeof(Archetype*));

  return newArch;
}

void* MoveRowBetweenArchetypes(Archetype* _src, Archetype* _dst, uint32_t _srcIndex)
{
  assert(_src && _dst);

  bool addingRow = _src->componentTypeCount < _dst->componentTypeCount;
  // Non-zero if either archetype has a component type count of 0
  uint8_t archHasNoComponents = _src->componentTypeCount == 0 || _dst->componentTypeCount == 0;
  uint32_t rowCount = addingRow ? _dst->componentTypeCount : _src->componentTypeCount;

  uint32_t srcTailIndex = (_src->indexToEntity.count > 0) ? _src->indexToEntity.count - 1 : 0;

  void* newComponentValue = NULL;
  void* srcTailValue = NULL;
  void* copiedValue = NULL;

  for (int32_t i = 0, j = 0; i < rowCount && j < rowCount;)
  {
    if (!archHasNoComponents && _src->pComponentTypes[i] == _dst->pComponentTypes[j])
    {
      copiedValue = DynamicArrayGet(&_src->pComponentArrays[i], _srcIndex);
      DynamicArrayPushBack(&_dst->pComponentArrays[j], copiedValue);

      srcTailValue = DynamicArrayGet(&_src->pComponentArrays[i], srcTailIndex);
      DynamicArraySet(&_src->pComponentArrays[i], _srcIndex, srcTailValue);
      DynamicArrayPopBack(&_src->pComponentArrays[i]);
      i++;
      j++;
    }
    else if (!addingRow)
    {
      // Component at src index [i] is the one being removed
      srcTailValue = DynamicArrayGet(&_src->pComponentArrays[i], srcTailIndex);
      DynamicArraySet(&_src->pComponentArrays[i], _srcIndex, srcTailValue);
      DynamicArrayPopBack(&_src->pComponentArrays[i]);
      i++;
    }
    else
    {
      // Component at dst index [j] is new.
      newComponentValue = DynamicArrayPushEmpty(&_dst->pComponentArrays[j]);
      j++;
    }
  }

  if (_src->indexToEntity.count > 0)
  {
    Entity srcTailEntity = *(Entity*)DynamicArrayGet(&_src->indexToEntity, srcTailIndex);
    EntityRecord* srcTailEntityRecord = (EntityRecord*)HashMapGet(&entityRecords, srcTailEntity);
    srcTailEntityRecord->index = _srcIndex;
    DynamicArrayPopBack(&_src->indexToEntity);
  }

  return newComponentValue;
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

// Creates a new archetype with components for all in _curArch with _component added to it
Archetype* BranchArchetypeAdd(Archetype* _curArch, ComponentId _component)
{
  // TODO : Make sure the archetype doesn't already have this component

  uint32_t newCount = _curArch->componentTypeCount + 1;
  ComponentId* newTypes = (ComponentId*)malloc(sizeof(ComponentId) * newCount);
  assert(newTypes);

  // Build new component list and maintain sorting
  ComponentId* oldTypes = _curArch->pComponentTypes;
  ComponentId heldId = _component;
  for (uint32_t i = 0; i < _curArch->componentTypeCount; i++)
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
  newTypes[_curArch->componentTypeCount] = heldId;

  // Test if this archetype already exists
  ArchetypeId archId = GetArchetypeId(newCount, newTypes);
  Archetype* newArchPtr = (Archetype*)HashMapGet(&archetypes, archId);

  if (newArchPtr == NULL)
  {
    Archetype newArch = CreateArchetype(newCount, newTypes);
    newArchPtr = (Archetype*)HashMapSet(&archetypes, newArch.id, &newArch);
  }

  HashMapSet(&_curArch->addArchetypes, newArchPtr->id, newArchPtr);
  HashMapSet(&newArchPtr->removeArchetypes, _curArch->id, _curArch);

  for (uint32_t i = 0; i < newCount; i++)
  {
    ComponentAddArchetype(newArchPtr->pComponentTypes[i], newArchPtr->id, i);
  }

  return newArchPtr;
}

// Creates a new archetype with components for all in _curArch with _component removed from it
Archetype* BranchArchetypeRemove(Archetype* _curArch, ComponentId _component)
{
  // TODO : Make sure the archetype actually has this component
  if (_curArch->componentTypeCount ==  0)
    return NULL;

  uint32_t newCount = _curArch->componentTypeCount - 1;
  ComponentId* newTypes = (ComponentId*)malloc(sizeof(ComponentId) * newCount);

  // Build new component list and maintain sorting
  for (uint32_t i = 0, j = 0; i < _curArch->componentTypeCount; i++)
  {
    if (_curArch->pComponentTypes[i] == _component)
    {
      continue;
    }

    newTypes[j] = _curArch->pComponentTypes[i];
    j++;
  }

  // Test if this archetype already exists
  ArchetypeId archId = GetArchetypeId(newCount, newTypes);
  Archetype* newArchPtr = (Archetype*)HashMapGet(&archetypes, archId);

  if (newArchPtr == NULL)
  {
    Archetype newArch = CreateArchetype(newCount, newTypes);
    newArchPtr = (Archetype*)HashMapSet(&archetypes, newArch.id, &newArch);
  }

  HashMapSet(&_curArch->addArchetypes, newArchPtr->id, newArchPtr);
  HashMapSet(&newArchPtr->removeArchetypes, _curArch->id, _curArch);

  for (uint32_t i = 0; i < newCount; i++)
  {
    ComponentAddArchetype(newArchPtr->pComponentTypes[i], newArchPtr->id, i);
  }

  return newArchPtr;
}

bool ArchetypeHasComponent(Archetype* arch, ComponentId component)
{
  uint32_t componentCount = arch->componentTypeCount;

  int32_t rangeStartIndex = 0;
  int32_t rangeEndIndex = componentCount - 1;
  uint32_t midIndex = 0;
  ComponentId midComponent = 0;

  while (rangeStartIndex <= rangeEndIndex)
  {
    midIndex = ((rangeEndIndex - rangeStartIndex) / 2) + rangeStartIndex;
    midComponent = arch->pComponentTypes[midIndex];

    if (midComponent == component)
    {
      return true;
    }
    else if (midComponent > component)
    {
      rangeEndIndex = midIndex - 1;
    }
    else
    {
      rangeStartIndex = midIndex + 1;
    }
  }

  return false;
}

void* DiamondEcsAddComponentById(Entity _entity, ComponentId _component)
{
  // TODO : Make sure the entity doesn't already have the component

  EntityRecord* record = (EntityRecord*)HashMapGet(&entityRecords, _entity);
  Archetype* curArch = record->archetype;
  Archetype* nextArch = (Archetype*)HashMapGet(&curArch->addArchetypes, _component);

  if (nextArch == NULL)
  {
    nextArch = BranchArchetypeAdd(curArch, _component);
  }

  void* valuePtr = MoveRowBetweenArchetypes(curArch, nextArch, record->index);

  record->archetype = nextArch;
  record->index = nextArch->pComponentArrays[0].count - 1;
  DynamicArrayPushBack(&nextArch->indexToEntity, &_entity);

  return valuePtr;
}

void DiamondEcsRemoveComponent(Entity _entity, const char* name)
{
  // TODO : Make sure the entity actually has the component
  ComponentId component = *(ComponentId*)HashMapStringGet(&ComponentTypeToId, name);

  EntityRecord* record = (EntityRecord*)HashMapGet(&entityRecords, _entity);
  Archetype* curArch = record->archetype;
  Archetype* nextArch = (Archetype*)HashMapGet(&curArch->removeArchetypes, component);

  if (nextArch == NULL)
  {
    nextArch = BranchArchetypeRemove(curArch, component);
  }

  MoveRowBetweenArchetypes(curArch, nextArch, record->index);

  record->archetype = nextArch;
  record->index = nextArch->pComponentArrays[0].count - 1;

  DynamicArrayPushBack(&nextArch->indexToEntity, &_entity);
}

void* DiamondEcsGetComponentById(Entity _entity, ComponentId _component)
{
  EntityRecord* record = (EntityRecord*)HashMapGet(&entityRecords, _entity);
  Archetype* arch = record->archetype;

  if (!ArchetypeHasComponent(arch, _component))
  {
    return NULL;
  }

  ComponentInfo* cinfo = (ComponentInfo*)HashMapGet(&componentInfos, _component);
  uint32_t* vptr = (uint32_t*)HashMapGet(&cinfo->archetypeMap, arch->id);
  assert(vptr);
  uint32_t componentIndex = *vptr;

  return DynamicArrayGet(&arch->pComponentArrays[componentIndex], record->index);
}

void* DiamondEcsGetComponent(Entity entity, const char* name)
{
  ComponentId component = *(ComponentId*)HashMapStringGet(&ComponentTypeToId, name);
  return DiamondEcsGetComponentById(entity, component);
}

void DiamondEcsSetComponent(Entity entity, const char* name, const void* value)
{
  ComponentId component = *(ComponentId*)HashMapStringGet(&ComponentTypeToId, name);

  // Get or add component to entity
  void* componentValue = DiamondEcsGetComponentById(entity, component);
  if (componentValue == NULL)
  {
    // Add component
    componentValue = DiamondEcsAddComponentById(entity, component);
  }

  // Set the value
  ComponentInfo* cinfo = (ComponentInfo*)HashMapGet(&componentInfos, component);
  memcpy(componentValue, value, cinfo->size);
}

// TODO : Define a component default value?
ComponentId DiamondEcsDefineComponent(const char* name, uint32_t _size)
{
  static uint32_t componentCount = 0;
  componentCount++;

  ComponentInfo cinfo = { 0 };
  cinfo.size = _size;
  cinfo.archetypeMap = HashMapInit(sizeof(uint32_t));

  HashMapSet(&componentInfos, componentCount, &cinfo);

  HashMapStringSet(&ComponentTypeToId, name, &componentCount);

  return componentCount;
}

Entity CreateEntity()
{
  entityCount++;

  EntityRecord newRecord = { 0 };
  newRecord.archetype = (Archetype*)HashMapGet(&archetypes, 0); // Default empty
  newRecord.index = 0; // Doesn't matter w/ the empty archetype

  HashMapSet(&entityRecords, entityCount, &newRecord);
  return entityCount;
}

void DestroyEntity(Entity _entity)
{
  // TODO : Recycle entity Ids

  EntityRecord* record = (EntityRecord*)HashMapGet(&entityRecords, _entity);

  // Remove the row from entity's current archetype
  Archetype* emptyArch = (Archetype*)HashMapGet(&archetypes, 0);
  MoveRowBetweenArchetypes(record->archetype, emptyArch, record->index);

  HashMapRemove(&entityRecords, _entity, NULL);
}

void PrintEntityComponents(Entity _entity)
{
  EntityRecord* rec = (EntityRecord*)HashMapGet(&entityRecords, _entity);
  if (rec == NULL)
  {
    printf("Cannot retrieve components for destroyed entity %u\n", _entity);
    return;
  }

  Archetype* arch = rec->archetype;

  printf("Components for Entity %u (archetype %u) : {", _entity, arch->id);
  for (uint32_t i = 0; i < arch->componentTypeCount; i++)
  {
    printf("%s%u", i == 0 ? "" : ", ", arch->pComponentTypes[i]);
  }
  printf("}\n");
}

void PrintArchetypeComponents(ArchetypeId _archId)
{
  Archetype* arch = (Archetype*)HashMapGet(&archetypes, _archId);
  if (arch == NULL)
  {
    printf("Cannot retrieve components for invalid archetype %u\n", _archId);
    return;
  }

  printf("  Components for Archetype %u : {", _archId);
  for (uint32_t i = 0; i < arch->componentTypeCount; i++)
  {
    printf("%s%u", i == 0 ? "" : ", ", arch->pComponentTypes[i]);
  }
  printf("}\n");
}

void PrintArchetypeEntities(ArchetypeId _archId)
{
  Archetype* arch = (Archetype*)HashMapGet(&archetypes, _archId);
  if (arch == NULL)
  {
    printf("Cannot retrieve components for invalid archetype %u\n", _archId);
    return;
  }

  printf("  Entities for Archetype %u : {", _archId);
  for (uint32_t i = 0; i < arch->indexToEntity.count; i++)
  {
    printf("%s%u", i == 0 ? "" : ", ", *((Entity*)DynamicArrayGet(&arch->indexToEntity, i)));
  }
  printf("}\n");
}

void PrintComponentArchetypes(ComponentId _component)
{
  ComponentInfo* cinfo = (ComponentInfo*)HashMapGet(&componentInfos, _component);

  HashElement* element = NULL;
  printf("Archetypes for Component %u : {", _component);
  for (uint32_t i = 0, printCount = 0; i < cinfo->archetypeMap.bucketCount; i++)
  {
    element = cinfo->archetypeMap.pBuckets[i];
    while (element != NULL)
    {
      printf("%s%u", printCount == 0 ? "" : ", ", element->key);
      printCount++;
      element = element->next;
    }
  }
  printf("}\n");
}

void PrintAllArchetypeComponents()
{
  HashElement* element = NULL;
  printf("Printing all archetype component lists :\n");
  for (uint32_t i = 0; i < archetypes.bucketCount; i++)
  {
    element = archetypes.pBuckets[i];
    while (element != NULL)
    {
      PrintArchetypeComponents(((Archetype*)element->value)->id);
      element = element->next;
    }
  }
}

void PrintAllArchetypeEntities()
{
  HashElement* element = NULL;
  printf("Printing all archetype entity lists :\n");
  for (uint32_t i = 0; i < archetypes.bucketCount; i++)
  {
    element = archetypes.pBuckets[i];
    while (element != NULL)
    {
      PrintArchetypeEntities(((Archetype*)element->value)->id);
      element = element->next;
    }
  }
}

void EcsStep()
{
  // Run all systems
  uint32_t systemCount = systems.count;
  SystemIterator iterator = { 0 };
  System* curSystem = NULL;
  iterator.archetypeCount = 0;

  for (uint32_t systemIndex = 0; systemIndex < systemCount; systemIndex++)
  {
    // Build the iterator

    // Call the iterator function for each component set found
    curSystem = (System*)DynamicArrayGet(&systems, systemIndex);
    curSystem->function(&iterator);
  }
}

void ExtractComponentIdsFromDefineString(const char* inString, uint32_t* outCount, ComponentId** outIds)
{
  *outCount = 0;

  const char* nameStart = inString;
  const char* nameEnd = inString;
  char nameBuffer[256] = { 'D', 'i', 'a', 'm', 'o', 'n', 'd', '_', 'E', 'c', 's', '_', 'I', 'D', '_'};
  uint32_t nameLength = 0;

  DynamicArray systemComponentIds = DynamicArrayInit(sizeof(ComponentId), 2);

  while (*nameStart != '\0')
  {
    while (*nameEnd != '\0' && *nameEnd != ',' && *nameEnd != ' ')
    {
      nameEnd++;
    }

    if (nameStart != nameEnd)
    {
      nameLength = nameEnd - nameStart;
      memcpy(nameBuffer + 15, nameStart, nameLength);
      nameBuffer[nameLength + 15] = '\0';

      printf("System adding component [%s]\n", nameBuffer);
      ComponentId* newId = (ComponentId*)HashMapStringGet(&ComponentTypeToId, nameBuffer);
      DynamicArrayPushBack(&systemComponentIds, newId);
    }

    nameStart = nameEnd + 1;
    nameEnd++;
  }

  uint32_t idsCount = systemComponentIds.count;
  ComponentId* idsArray = (ComponentId*)malloc(sizeof(ComponentId) * idsCount);
  memcpy(idsArray, systemComponentIds.data, sizeof(ComponentId) * idsCount);

  *outCount = idsCount;
  *outIds = idsArray;

  DynamicArrayShutdown(&systemComponentIds);
}

void DiamondEcsDefineSystem(EcsSystemFunction function, const char* componentNames)
{
  System newSystem = { 0 };

  ExtractComponentIdsFromDefineString(componentNames, &newSystem.componetCount, &newSystem.componentIds);
  if (newSystem.componetCount == 0)
  {
    // TODO : Fatal failure
    return;
  }

  for (uint32_t i = 0; i < newSystem.componetCount; i++)
  {
    printf("Component[%u] = %u\n", i, newSystem.componentIds[i]);
  }

  newSystem.function = function;

  DynamicArrayPushBack(&systems, &newSystem);
}

void* DiamondEcsGetSystemComponent(SystemIterator* iterator, const char* componentName)
{
  

  return NULL;
}
