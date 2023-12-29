#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

namespace Diamond
{

class StepVector
{
public:
  StepVector() { m_data = nullptr; }
  ~StepVector()
  {
    //if (m_data != nullptr)
    //  free(m_data);
  }

  void Init(uint32_t elementSize)
  {
    m_elementSize = elementSize;
    m_count = 0;
    if (elementSize > 0)
    {
      m_capacity = 1;
      m_data = malloc(m_elementSize * m_capacity);
    }
    else
    {
      m_capacity = 0;
      m_data = nullptr;
    }
  }

  void* PushBack(const void* data = nullptr)
  {
    if (m_count == m_capacity)
    {
      m_capacity *= 2;
      m_data = realloc(m_data, m_elementSize * m_capacity);
    }

    if (data != nullptr)
    {
      memcpy(((char*)m_data) + m_count, data, m_elementSize);
    }

    m_count++;
    return (void*)(((char*)m_data) + (m_elementSize * (m_count - 1)));
  }

  void* PopBack()
  {
    if (m_count == 0)
      return nullptr;

    m_count--;
    return (void*)(((char*)m_data) + (m_elementSize * m_count));
  }

  void Resize(uint32_t newElementCount)
  {
    if (newElementCount > m_capacity)
    {
      m_capacity = newElementCount;
      m_data = realloc(m_data, m_elementSize * m_capacity);
    }

    if (newElementCount < m_count)
    {
      m_count = newElementCount;
    }
  }

  uint32_t elementCount()
  {
    return m_count;
  }

  uint32_t elementSize()
  {
    return m_elementSize;
  }

  void* operator[](uint32_t index)
  {
    assert(index < m_count);
    return (void*)((char*)m_data + (index * m_elementSize));
  }

  void* GetSubElement(uint32_t elementIndex, uint32_t offset)
  {
    return (void*)((char*)m_data + ((elementIndex * m_elementSize) + offset));
  }

private:
  uint32_t m_elementSize;
  void* m_data;
  uint32_t m_count;
  uint32_t m_capacity;
};

} // namespace Diamond
