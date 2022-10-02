#pragma once
#include <prism/blitz.h>

Screen* getRunnerScreen();

MugenSpriteFile* getRunnerScreenSprites();
MugenAnimations* getRunnerScreenAnimations();

int getRunningScreenScale();
void loadRunnerScreenSpritesPlusAnimationsSolo();