
#include "ecs.h"

#include <stdio.h>
#include <math.h>

typedef struct Position
{
  float x, y;
} Position;

typedef struct Velocity
{
  float x, y;
} Velocity;

int main()
{
  EcsWorld* ecs = EcsInit();
  if (!ecs)
    return -1;

  EcsDefineComponent(ecs, Position);
  EcsDefineComponent(ecs, Velocity);

  const uint32_t entityCount = 5;
  EcsEntity entities[5];

  for (uint32_t i = 0; i < entityCount; i++)
  {
    entities[i] = EcsCreateEntity(ecs);
    float theta = i * ((3.14f * 2.0f) / entityCount);
    EcsSet(ecs, entities[i], Position, { 0.0f, 0.0f });
    EcsSet(ecs, entities[i], Velocity, { cosf(theta + 0.25f), sinf(theta + 0.25f) });
  }

  char printBuffer[1024] = { 0 };
  while (1)
  {
    uint32_t count;
    Velocity* pVel = EcsGetComponentList(ecs, Velocity, NULL);
    Position* pPos = EcsGetComponentList(ecs, Position, &count);

    printBuffer[0] = '\0';

    for (uint32_t i = 0; i < count; i++)
    {
      if (pPos[i].x < 0.0f || pPos[i].x > 10.0f)
        pVel[i].x *= -1;

      if (pPos[i].y < 0.0f || pPos[i].y > 10.0f)
        pVel[i].y *= -1;

      pPos[i].x += pVel[i].x * 0.0001f;
      pPos[i].y += pVel[i].y * 0.0001f;

      sprintf(printBuffer, "%s%d : Pos(%8.4f, %8.4f) Vel(%8.4f, %8.4f)\n", printBuffer, i, pPos[i].x, pPos[i].y, pVel[i].x, pVel[i].y);
    }

    printf("%s\n", printBuffer);
  }

  return 0;
}
