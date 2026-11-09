/*
 * end_plan.c - countdown end plan. See end_plan.h.
 */
#include "end_plan.h"
#include <string.h>

static const char *const scene_names[SCENE_COUNT] = { "none", "fireworks", "stars", "confetti", "snow", "rainbow" };
static const char *const then_names[END_THEN_COUNT] = { "hold", "screen" };

EndPhase EndPlan_Phase(const EndPlan *p, uint32_t secs_since_zero)
{
  if (p->scene != SCENE_NONE && (p->scene_s == 0u || secs_since_zero < p->scene_s))
  {
    return END_PHASE_SCENE;
  }
  return END_PHASE_THEN;
}

const char *EndPlan_SceneName(SceneId s) { return s < SCENE_COUNT ? scene_names[s] : "?"; }
const char *EndPlan_ThenName(EndThen t)  { return t < END_THEN_COUNT ? then_names[t] : "?"; }

int EndPlan_SceneFind(const char *name)
{
  for (int i = 0; i < SCENE_COUNT; i++)
  {
    if (strcmp(name, scene_names[i]) == 0) return i;
  }
  return -1;
}

int EndPlan_ThenFind(const char *name)
{
  for (int i = 0; i < END_THEN_COUNT; i++)
  {
    if (strcmp(name, then_names[i]) == 0) return i;
  }
  return -1;
}

int EndPlan_Valid(const EndPlan *p)
{
  return p->scene < SCENE_COUNT && p->then < END_THEN_COUNT;
}
