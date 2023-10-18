
#include <stdio.h>
#include <stdint.h>

#include "ecs.h"

typedef struct foo
{
  float x, y, z;
} foo;

typedef struct bar
{
  int a;
} bar;

void TestSystem(SystemIterator* iterator)
{
  static uint32_t count = 0;
  printf("Test system : %u\n", count++);
}

int main()
{
  EcsInit();

  EcsDefineComponent(uint32_t);
  EcsDefineComponent(foo);
  EcsDefineComponent(bar);

  Entity e1 = CreateEntity();
  Entity e2 = CreateEntity();

  EcsSet(e1, uint32_t, {1231});
  // PrintAllArchetypeEntities();
  EcsSet(e2, foo, { 20.0f, 22.55f, 2002.5587f });
  // PrintAllArchetypeEntities();

  EcsSet(e1, foo, { 10.0f, 12.55f, 1002.5587f });
  // PrintAllArchetypeEntities();
  EcsSet(e2, uint32_t, {1114458});
  // PrintAllArchetypeEntities();

  EcsSet(e2, bar, {-123454321});
  // PrintAllArchetypeEntities();

  uint32_t tmpInt = *EcsGet(e2, uint32_t);
  printf("> val e2 : %u\n", tmpInt);
  foo tmpFoo = *EcsGet(e2, foo);
  printf("> foo e2 : %f, %f, %f\n", tmpFoo.x, tmpFoo.y, tmpFoo.z);
  bar tmpBar = *EcsGet(e2, bar);
  printf("> bar e2 : %d\n", tmpBar.a);

  EcsDefineSystem(TestSystem, foo, bar);

  EcsRemove(e2, foo);
  // PrintAllArchetypeEntities();
  DestroyEntity(e1);
  // PrintAllArchetypeEntities();

  EcsStep();
  EcsStep();
  EcsStep();

  EcsShutdown();
  return 0;
}
