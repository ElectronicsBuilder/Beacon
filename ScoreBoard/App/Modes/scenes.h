/*
 * scenes.h - full-screen animations (fireworks, stars, confetti, snow,
 * rainbow), e.g. when the countdown reaches zero. About 30 frames/s.
 * An optional text is drawn over the animation on the lower text line.
 */
#ifndef APP_SCENES_H
#define APP_SCENES_H

#include "end_plan.h"

#define TEXT_OVERLAY_MAX 48

void Scene_Start(SceneId id, const char *text);   /* clears the screen */
void Scene_Tick(void);                             /* every 1 ms */

#endif
