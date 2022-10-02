#pragma once
#include <prism/blitz.h>

ActorBlueprint getRunnerPlayer();

void setPlayerHitBySpike();
void setPlayerHitByBounce();
void setPlayerHitByAccelerate();

void setPlayerSize(double s);
void setPlayerGravity(double s);
void setPlayerInvincible(int isInv);
void reversePlayerDirection(int isRev);
void setPlayerBlind();
void setPlayerUnblind();

CollisionListData* getRunnerPlayerPlatformCollisionList();
CollisionListData* getRunnerPlayerEnemyCollisionList();