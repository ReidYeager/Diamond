#pragma once

// Implementation of an Entity-Component-System archetecture
// Revolves around the idea of component "archetypes"
// Archetypes may only contain one instance of each component
//
// TODO : Rewrite to better utilize memory.
// > Currently requires 2^n archetypes for an entity with n components attached

#include "containers/stepvector.h"

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>
#include <queue>
#include <deque>
#include <string>
#include <stdio.h>
#include <stdint.h>

namespace Diamond
{

#define DMD_PRINT(message, ...) printf(message"\n", __VA_ARGS__)

typedef uint32_t ArchetypeId;
typedef uint32_t ComponentId;
typedef uint32_t Entity;

struct EcsArchetype
{
  ArchetypeId id;                           // Hash of constituent componentId's
  std::vector<Entity> owningEntities;       // Entity that "owns" each index
  std::vector<ComponentId> componentIds;    // For initializing subtractional edges

  // TODO : Profile interleaved component data vs separate comonent arrays
  Diamond::StepVector componentData;         // Memory for all components' data interleaved for each entity
  std::unordered_map<ComponentId, uint32_t> componentDataOffsets;

  std::unordered_map<ComponentId, EcsArchetype*> additionalEdges;    // Archetypes with 'ComponentId' added
  std::unordered_map<ComponentId, EcsArchetype*> subtractionalEdges; // Archetypes with 'ComponentId' removed
};

struct EcsRecord
{
  EcsArchetype* archetype;
  uint32_t index;
};

class EcsWorld
{
public:
  friend class EcsIterator;

  EcsWorld();
  ~EcsWorld();

  Entity CreateEntity();
  void DestroyEntity(Entity e);

  ComponentId GetComponentId(const char* name);
  ComponentId DefineComponent(const char* name, uint32_t size);
  void* AddComponent(Entity e, ComponentId id);
  void* SetComponent(Entity e, ComponentId id, const void* data);
  void* GetComponent(Entity e, ComponentId id);
  void RemoveComponent(Entity e, ComponentId id);
  bool EntityHasComponent(Entity e, ComponentId id);
  bool EntityIsAlive(Entity e);

  void PrintNextEdges();
  void PrintPrevEdges();
  void PrintArchOwners();

private:
  Entity m_nextEntity = 1; // Entity value 0 invalid
  ComponentId m_nextComponentId = 1; // ComponentId value 0 invalid
  std::unordered_map<ComponentId, std::string> m_componentNames;
  std::unordered_map<ComponentId, uint32_t> m_componentSizes;
  std::unordered_map<ArchetypeId, EcsArchetype> m_archetypes;
  std::unordered_map<Entity, EcsRecord> m_records; // NOTE : Could probably just be an array depending upon how Entity is structured

  ArchetypeId GetArchetypeId(const std::vector<ComponentId>& components);
  bool ArchetypeHasComponent(EcsArchetype* arch, ComponentId id);
  EcsArchetype* CreateArchetype(const std::vector<ComponentId>& components);
  EcsArchetype* GetArchetypeNext(EcsArchetype* arch, ComponentId id);
  EcsArchetype* GetArchetypePrevious(EcsArchetype* arch, ComponentId id);
  void* GetComponentData(EcsRecord* record, ComponentId id);
};

class EcsIterator
{
public:
  EcsIterator(EcsWorld* world, const std::vector<ComponentId>& components) : m_world(world), m_components(components)
  {
    if (components.size() == 0)
    {
      return;
    }

    std::vector<ComponentId> ids(components);

    for (uint32_t i = 0; i < ids.size() - 1; i++)
    {
      for (uint32_t j = i + 1; j < ids.size(); j++)
      {
        if (ids[i] > ids[j])
        {
          ComponentId tmp = ids[j];
          ids[j] = ids[i];
          ids[i] = tmp;
        }
      }
    }

    ArchetypeId startArchId = world->GetArchetypeId(ids);
    if (!world->m_archetypes.contains(startArchId))
    {
      m_currentArch = nullptr;
      m_currentLayerQueue.clear();
      m_nextLayerQueue.clear();
      return;
    }

    m_currentArch = &world->m_archetypes[startArchId];

    SetupCurrentArch();
  }

  bool StepNextElement();
  bool AtEnd();

  Entity CurrentEntity();

  void* GetComponent(ComponentId id);

private:
  EcsWorld* m_world;
  const std::vector<ComponentId> m_components;

  std::deque<EcsArchetype*> m_currentLayerQueue, m_nextLayerQueue;
  EcsArchetype* m_currentArch;
  uint32_t m_currentArchElementIndex;

  void SetupCurrentArch();
};

// TODO : Iterator that traverses archetypes for use by systems

} // namespace Diamond
