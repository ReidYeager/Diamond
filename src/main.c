
#include "ecs.h"

#include <stdio.h>

typedef struct Position
{
  float x, y, z;
} Position;

int main()
{
  EcsWorld* ecs = EcsInit();
  if (!ecs)
    return -1;

  EcsEntity a = EcsCreateEntity(ecs);

  EcsDefineComponent(ecs, Position);

  EcsSet(ecs, a, Position, { 1, 2, 3 });

  Position* p = EcsGet(ecs, a, Position);
  printf("A has : %f, %f, %f\n", p->x, p->y, p->z);

  return 0;
}
