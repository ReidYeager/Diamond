
#include "ecs.h"

#include <stdio.h>

struct Position
{
  float x, y, z;
};

int main()
{
  EcsWorld* ecs = EcsInit();
  if (!ecs)
    return -1;

  EcsEntity a = EcsCreateEntity(ecs);
  EcsEntity b = EcsCreateEntity(ecs);
  EcsEntity c = EcsCreateEntity(ecs);
  EcsDestroyEntity(ecs, b);
  EcsEntity d = EcsCreateEntity(ecs);
  EcsEntity e = EcsCreateEntity(ecs);
  EcsDestroyEntity(ecs, a);
  EcsDestroyEntity(ecs, d);
  EcsEntity f = EcsCreateEntity(ecs);
  EcsEntity g = EcsCreateEntity(ecs);

  return 0;
}
