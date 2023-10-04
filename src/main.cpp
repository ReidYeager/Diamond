
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

  EcsDefineComponent(ecs, Position);

  return 0;
}
