
#include "ecs.h"

#include <stdio.h>

typedef struct Position
{
  int x, y, z;
} Position;

int main()
{
  EcsWorld* ecs = EcsInit();
  if (!ecs)
    return -1;

  EcsDefineComponent(ecs, Position);

  EcsEntity entities[8];

  entities[0] = EcsCreateEntity(ecs);
  entities[1] = EcsCreateEntity(ecs);
  entities[2] = EcsCreateEntity(ecs);
  entities[3] = EcsCreateEntity(ecs);

  EcsSet(ecs, entities[0], Position, { 1, 2, 3 });
  EcsSet(ecs, entities[1], Position, { 4, 5, 6 });
  EcsSet(ecs, entities[2], Position, { 7, 8, 9 });
  EcsSet(ecs, entities[3], Position, { 10, 11, 12 });

  EcsDestroyEntity(ecs, entities[1]);

  entities[4] = EcsCreateEntity(ecs);

  EcsSet(ecs, entities[4], Position, { 13, 14, 15 });

  for (uint32_t i = 0; i < 5; i++)
  {
    const Position* p = EcsGet(ecs, entities[i], Position);
    if (p)
      printf("%d : %d, %d, %d\n", i, p->x, p->y, p->z);
  }

  

  return 0;
}
