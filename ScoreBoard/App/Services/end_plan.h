/*
 * end_plan.h - what the countdown shows after it reaches zero.
 * Pure logic (PC tests); stored and used by Modes/mode_countdown.c.
 *
 *   1. scene (optional): a full-screen animation for scene_s seconds
 *      (0 = until the countdown is changed), with the end text over it
 *   2. then: hold   = 00:00:00 in red with the end text (the countdown screen)
 *            screen = the "end" screen layout ("screen end ..."): text,
 *                     its messages ("screen end msgs ..."), time since zero
 */
#ifndef APP_END_PLAN_H
#define APP_END_PLAN_H

#include <stdint.h>

typedef enum { SCENE_NONE, SCENE_FIREWORKS, SCENE_STARS, SCENE_CONFETTI, SCENE_SNOW, SCENE_RAINBOW,
               SCENE_CODERAIN, SCENE_PLASMA, SCENE_FIRE, SCENE_WARP, SCENE_HEARTS, SCENE_BALLOONS,
               SCENE_COUNT } SceneId;   /* new ones at the end: the number is stored */
typedef enum { END_THEN_HOLD, END_THEN_SCREEN, END_THEN_COUNT } EndThen;
typedef enum { END_PHASE_SCENE, END_PHASE_THEN } EndPhase;

typedef struct {
  uint8_t  scene;          /* SceneId */
  uint8_t  then;           /* EndThen */
  uint16_t scene_s;        /* seconds of the scene, 0 = no end */
} EndPlan;

#define END_PLAN_DEFAULT { SCENE_NONE, END_THEN_HOLD, 30u }

EndPhase EndPlan_Phase(const EndPlan *p, uint32_t secs_since_zero);
const char *EndPlan_SceneName(SceneId s);
int  EndPlan_SceneFind(const char *name);        /* SceneId or -1 */
const char *EndPlan_ThenName(EndThen t);
int  EndPlan_ThenFind(const char *name);         /* EndThen or -1 */
int  EndPlan_Valid(const EndPlan *p);

#endif
