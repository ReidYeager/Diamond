
#include "ecs.h"

namespace Diamond
{

EcsWorld::EcsWorld()
{
  CreateArchetype({});
}

EcsWorld::~EcsWorld()
{
  m_componentNames.clear();
  m_componentSizes.clear();
  m_archetypes.clear();
  m_records.clear();
}

ComponentId EcsWorld::GetComponentId(const char* name)
{
  uint32_t hash = 0;
  for (uint32_t i = 0; name[i] != '\0'; i++)
  {
    hash = 31 * hash + name[i];
  }

  return hash;
}

ArchetypeId EcsWorld::GetArchetypeId(const std::vector<ComponentId>& components)
{
  // TODO : Sort the component list
  uint32_t hash = 0;
  for (uint32_t i = 0; i < components.size(); i++)
  {
    hash = 31 * hash + components[i];
  }

  return hash;
}

bool EcsWorld::EntityHasComponent(Entity e, ComponentId id)
{
  EcsRecord& record = m_records[e];
  return ArchetypeHasComponent(record.archetype, id);
}

bool EcsWorld::ArchetypeHasComponent(EcsArchetype* arch, ComponentId id)
{
  for (uint32_t i = 0; i < arch->componentIds.size(); i++)
  {
    if (arch->componentIds[i] == id)
      return true;
  }
  return false;
}

// ==========================================================================================
// Entity
// ==========================================================================================

Entity EcsWorld::CreateEntity()
{
  EcsRecord* record = &m_records[m_nextEntity];
  record->archetype = &m_archetypes[0];
  record->index = 0;

  record->archetype->owningEntities.push_back(m_nextEntity);

  return m_nextEntity++;
}

void EcsWorld::DestroyEntity(Entity e)
{
  EcsRecord* record = &m_records[e];
  EcsArchetype* arch = record->archetype;
  uint32_t index = record->index;
  uint32_t backIndex = arch->owningEntities.size() - 1;
  Entity backEntity = arch->owningEntities[backIndex];

  if (backIndex >= 0)
  {
    if (arch->componentData.elementCount() > 0 && backEntity != e)
    {
      EcsRecord* backRecord = &m_records[backEntity];

      void* data = arch->componentData[index];
      void* backData = arch->componentData[backIndex];
      memcpy(data, backData, arch->componentData.elementSize());

      arch->owningEntities[index] = backEntity;
      backRecord->index = index;
    }

    arch->componentData.PopBack();
    arch->owningEntities.pop_back();
  }
}

// ==========================================================================================
// Component
// ==========================================================================================

ComponentId EcsWorld::DefineComponent(const char* name, uint32_t size)
{
  ComponentId id = GetComponentId(name);
  m_componentNames[id] = std::string(name);
  m_componentSizes[id] = size;

  return id;
}

void* EcsWorld::AddComponent(Entity e, ComponentId id)
{
  if (m_componentSizes.find(id) == m_componentSizes.end())
  {
    assert(false && "Component not defined");
  }

  EcsRecord* record = &m_records[e];
  EcsArchetype* src = record->archetype;
  EcsArchetype* dst = GetArchetypeNext(record->archetype, id);
  uint32_t srcElementIndex = record->index;
  uint32_t dstElementIndex = dst->componentData.elementCount();

  record->archetype = dst;
  record->index = dstElementIndex;

  dst->owningEntities.push_back(e);
  void* dstData = dst->componentData.PushBack();

  // Copy data to next archetype
  void* srcData = nullptr;
  if (src->componentData.elementCount() > 0)
  {
    srcData = src->componentData[srcElementIndex];
  }
  uint32_t srcIndex = 0, dstIndex = 0;
  uint32_t srcOffset = 0, dstOffset = 0;
  uint32_t curSize = 0;
  for (; dstIndex < dst->componentIds.size();)
  {
    curSize = m_componentSizes[dst->componentIds[dstIndex]];

    if (srcIndex >= src->componentIds.size() || src->componentIds[srcIndex] != dst->componentIds[dstIndex])
    {
      memset(((char*)dstData) + dstOffset, 0, curSize);
      dstOffset += curSize;
      dstIndex++;
      continue;
    }

    memcpy(((char*)dstData) + dstOffset, ((char*)srcData) + srcOffset, curSize);

    srcOffset += curSize;
    dstOffset += curSize;
    srcIndex++;
    dstIndex++;
  }

  // Remove entity from src archetype
  int32_t backIndex = src->owningEntities.size() - 1;
  if (backIndex >= 0)
  {
    Entity backEntity = src->owningEntities[backIndex];
    EcsRecord* backRecord = &m_records[backEntity];

    if (src->componentData.elementCount() > 0 && backEntity != e)
    {
      void* backData = src->componentData[backIndex];
      memcpy(srcData, backData, src->componentData.elementSize());

      src->owningEntities[srcElementIndex] = backEntity;
      backRecord->index = srcElementIndex;
    }
    src->componentData.PopBack();
    src->owningEntities.pop_back();
  }

  return dst->componentData.GetSubElement(dstElementIndex, dst->componentDataOffsets[id]);
}

void* EcsWorld::SetComponent(Entity e, ComponentId id, const void* data)
{
  void* componentData = nullptr;
  if (!EntityHasComponent(e, id))
  {
    componentData = AddComponent(e, id);
  }
  else
  {
    componentData = GetComponent(e, id);
  }

  if (data != nullptr && componentData != nullptr)
  {
    memcpy(componentData, data, m_componentSizes[id]);
  }

  return componentData;
}

void* EcsWorld::GetComponent(Entity e, ComponentId id)
{
  void* componentData = nullptr;
  if (!EntityHasComponent(e, id))
  {
    return nullptr;
  }

  EcsRecord* record = &m_records[e];
  uint32_t offset = record->archetype->componentDataOffsets[id];
  return record->archetype->componentData.GetSubElement(record->index, offset);
}

void EcsWorld::RemoveComponent(Entity e, ComponentId id)
{
  EcsRecord* record = &m_records[e];
  EcsArchetype* src = record->archetype;
  EcsArchetype* dst = GetArchetypePrevious(record->archetype, id);
  uint32_t srcElementIndex = record->index;
  uint32_t dstElementIndex = dst->componentData.elementCount();

  record->archetype = dst;
  record->index = dstElementIndex;

  dst->owningEntities.push_back(e);
  void* dstData = dst->componentData.PushBack();

  // Copy data to previous archetype
  void* srcData = src->componentData[srcElementIndex];
  uint32_t srcIndex = 0, dstIndex = 0;
  uint32_t srcOffset = 0, dstOffset = 0;
  uint32_t curSize = 0;
  for (; srcIndex < src->componentIds.size();)
  {
    curSize = m_componentSizes[src->componentIds[srcIndex]];

    if (dstIndex >= dst->componentIds.size() || src->componentIds[srcIndex] != dst->componentIds[dstIndex])
    {
      srcOffset += curSize;
      srcIndex++;
      continue;
    }

    memcpy(((char*)dstData) + dstOffset, ((char*)srcData) + srcOffset, curSize);

    srcOffset += curSize;
    dstOffset += curSize;
    srcIndex++;
    dstIndex++;
  }

  // Remove entity from src archetype
  int32_t backIndex = src->owningEntities.size() - 1;
  if (backIndex >= 0)
  {
    Entity backEntity = src->owningEntities[backIndex];
    EcsRecord* backRecord = &m_records[backEntity];

    if (src->componentData.elementCount() > 0 && backEntity != e)
    {
      void* backData = src->componentData[backIndex];
      memcpy(srcData, backData, src->componentData.elementSize());

      src->owningEntities[srcElementIndex] = backEntity;
      backRecord->index = srcElementIndex;
    }
    src->componentData.PopBack();
    src->owningEntities.pop_back();
  }
}

// ==========================================================================================
// Archetype
// ==========================================================================================

EcsArchetype* EcsWorld::CreateArchetype(const std::vector<ComponentId>& components)
{
  ArchetypeId id = GetArchetypeId(components);

  if (m_archetypes.contains(id))
  {
    return &m_archetypes[id];
  }

  EcsArchetype* newArchetype = &m_archetypes[id];

  newArchetype->id = id;
  newArchetype->componentIds = components;
  newArchetype->owningEntities = {};
  newArchetype->additionalEdges = {};
  newArchetype->subtractionalEdges = {};

  uint32_t sizeSum = 0;
  for (uint32_t i = 0; i < components.size(); i++)
  {
    newArchetype->componentDataOffsets[components[i]] = sizeSum;
    sizeSum += m_componentSizes[components[i]];
  }
  newArchetype->componentData.Init(sizeSum);

  // Connect edges to other archetypes -- prevents any nodes from remaining unconnected
  // TODO : This may cause some hitching issues later. Profile it.
  for (uint32_t i = 0; i < components.size(); i++)
  {
    ComponentId removedId = components[i];
    EcsArchetype* prevArch = GetArchetypePrevious(newArchetype, removedId);
    newArchetype->subtractionalEdges[removedId] = prevArch;
    prevArch->additionalEdges[removedId] = newArchetype;
  }

  return newArchetype;
}

EcsArchetype* EcsWorld::GetArchetypeNext(EcsArchetype* arch, ComponentId id)
{
  if (ArchetypeHasComponent(arch, id))
    return arch;

  if (!arch->additionalEdges.contains(id))
  {
    std::vector<ComponentId> nextIds(arch->componentIds);
    nextIds.push_back(id);

    // Keep sorted
    for (int32_t i = nextIds.size() - 2; i >= 0; i--)
    {
      if (nextIds[i] > id)
      {
        nextIds[i + 1] = nextIds[i];
        nextIds[i] = id;
      }
      else
      {
        break;
      }
    }

    arch->additionalEdges[id] = CreateArchetype(nextIds);
  }

  return arch->additionalEdges[id];
}

EcsArchetype* EcsWorld::GetArchetypePrevious(EcsArchetype* arch, ComponentId id)
{
  if (!ArchetypeHasComponent(arch, id))
    return arch;

  if (arch->subtractionalEdges.contains(id))
  {
    return arch->subtractionalEdges[id];
  }

  if (arch->componentIds.size() == 0)
    return nullptr;

  std::vector<ComponentId> prevIds(arch->componentIds);

  ComponentId prevId = prevIds[prevIds.size() - 1];
  if (prevId != id)
  {
    for (int32_t i = prevIds.size() - 2; i >= 0; i--)
    {
      if (prevIds[i] == id)
      {
        prevIds[i] = prevId;
        break;
      }

      ComponentId tmp = prevIds[i];
      prevIds[i] = prevId;
      prevId = tmp;
    }
  }
  prevIds.pop_back();

  EcsArchetype* newArch = CreateArchetype(prevIds);
  arch->subtractionalEdges[id] = newArch;
  return newArch;
}

void* EcsWorld::GetComponentData(EcsRecord* record, ComponentId id)
{
  char* data = (char*)record->archetype->componentData[record->index];
  return (data + record->archetype->componentDataOffsets[id]);
}

// ==========================================================================================
// Debug prints
// ==========================================================================================

void EcsWorld::PrintNextEdges()
{
  char componentNamesBuffer[1024];

  for (std::pair<ArchetypeId, EcsArchetype> ap : m_archetypes)
  {
    EcsArchetype& arch = ap.second;
    componentNamesBuffer[0] = '\0';

    for (uint32_t i = 0; i < arch.componentIds.size(); i++)
    {
      sprintf(componentNamesBuffer, "%s, %s", componentNamesBuffer, m_componentNames[arch.componentIds[i]].c_str());
    }

    DMD_PRINT("Arch (%s)\n\t>%zu", componentNamesBuffer, arch.additionalEdges.size());
    for (std::pair<ComponentId, EcsArchetype*> edge : arch.additionalEdges)
    {
      DMD_PRINT(">> %u", edge.second->id);
    }
  }
}

void EcsWorld::PrintPrevEdges()
{
  char componentNamesBuffer[1024];

  for (std::pair<ArchetypeId, EcsArchetype> ap : m_archetypes)
  {
    EcsArchetype& arch = ap.second;
    componentNamesBuffer[0] = '\0';

    for (uint32_t i = 0; i < arch.componentIds.size(); i++)
    {
      sprintf(componentNamesBuffer, "%s, %s", componentNamesBuffer, m_componentNames[arch.componentIds[i]].c_str());
    }

    DMD_PRINT("Arch (%s)\n\t>%zu", componentNamesBuffer, arch.subtractionalEdges.size());
    for (std::pair<ComponentId, EcsArchetype*> edge : arch.subtractionalEdges)
    {
      DMD_PRINT(">> %u", edge.second->id);
    }
  }
}

void EcsWorld::PrintArchOwners()
{
  char componentNamesBuffer[1024];
  char ownersBuffer[1024];

  for (std::pair<ArchetypeId, EcsArchetype> ap : m_archetypes)
  {
    ownersBuffer[0] = '\0';
    EcsArchetype& arch = ap.second;
    componentNamesBuffer[0] = '\0';

    for (uint32_t i = 0; i < arch.componentIds.size(); i++)
    {
      sprintf(componentNamesBuffer, "%s, %s", componentNamesBuffer, m_componentNames[arch.componentIds[i]].c_str());
    }

    for (uint32_t i = 0; i < arch.owningEntities.size(); i++)
    {
      sprintf(ownersBuffer, "%s, %u", ownersBuffer, arch.owningEntities[i]);
    }

    DMD_PRINT("Arch (%s)\n\t>%s", componentNamesBuffer, ownersBuffer);
  }
}

// ==========================================================================================
// Iterator
// ==========================================================================================

bool EcsIterator::StepNextElement()
{
  m_currentArchElementIndex++;

  if (m_currentArchElementIndex >= m_currentArch->componentData.elementCount())
  {
    m_currentArchElementIndex = 0;
    m_currentArch = nullptr;

    if (m_currentLayerQueue.empty())
    {
      m_currentLayerQueue = std::deque<EcsArchetype*>(m_nextLayerQueue);
      m_nextLayerQueue.clear();
    }

    if (m_currentLayerQueue.empty())
    {
      return true;
    }

    m_currentArch = m_currentLayerQueue.front();
    m_currentLayerQueue.pop_front();

    SetupCurrentArch();
  }

  return AtEnd();
}

bool EcsIterator::AtEnd()
{
  return m_currentLayerQueue.empty() && m_nextLayerQueue.empty() && m_currentArch == nullptr;
}

Entity EcsIterator::CurrentEntity()
{
  return m_currentArch->owningEntities[m_currentArchElementIndex];
}

void* EcsIterator::GetComponent(ComponentId id)
{
  return m_currentArch->componentData.GetSubElement(m_currentArchElementIndex, m_currentArch->componentDataOffsets[id]);
}

bool EcsIterator::HasComponent(ComponentId id)
{
  return std::find(m_currentArch->componentIds.begin(), m_currentArch->componentIds.end(), id) != m_currentArch->componentIds.end();
}

void EcsIterator::SetupCurrentArch()
{
  std::deque<EcsArchetype*>::iterator iter;
  for (std::pair<ComponentId, EcsArchetype*> child : m_currentArch->additionalEdges)
  {
    iter = std::find(m_nextLayerQueue.begin(), m_nextLayerQueue.end(), child.second);
    if (iter == m_nextLayerQueue.end())
    {
      m_nextLayerQueue.push_back(child.second);
    }
  }

  if (m_currentArch->owningEntities.size() == 0)
  {
    StepNextElement();
    return;
  }

  m_currentArchElementIndex = 0;
}

} // namespace Diamond
