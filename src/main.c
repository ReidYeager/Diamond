
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

int main()
{
  EcsInit();

  ComponentId intComponent = DefineComponent(sizeof(uint32_t));
  ComponentId fooComponent = DefineComponent(sizeof(foo));
  ComponentId barComponent = DefineComponent(sizeof(bar));

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

  PrintEntityComponents(e2);

  f = (foo*)AddComponent(e2, fooComponent);
  f->x = 20.0f;
  f->y = 22.55f;
  f->z = 2002.558f;

  PrintEntityComponents(e2);

  v = (uint32_t*)AddComponent(e2, intComponent);

  PrintEntityComponents(e2);

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

  AddComponent(e2, barComponent);
  PrintEntityComponents(e2);
  RemoveComponent(e2, fooComponent);
  PrintEntityComponents(e2);

  EcsShutdown();
  return 0;
}
