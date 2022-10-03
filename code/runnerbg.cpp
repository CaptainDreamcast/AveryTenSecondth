#include "runnerbg.h"
#include "runnerscreen.h"

static struct {
	int bgEntityId;
	int bgEntityIdSwap;

	CollisionListData* floorCollisionList;
	int collisionEntityId;
	int floorCollisionId;

} gRunnerBGData;

class RunnerBG {
public:
	Vector2DI spriteSize;

	RunnerBG() {
		gRunnerBGData.bgEntityId = addBlitzEntity(Position(0, 0, 1));
		addBlitzMugenAnimationComponent(gRunnerBGData.bgEntityId, getRunnerScreenSprites(), getRunnerScreenAnimations(), 1);
		addBlitzPhysicsComponent(gRunnerBGData.bgEntityId);
		addBlitzPhysicsVelocityX(gRunnerBGData.bgEntityId, -3.f);
		spriteSize = getAnimationFirstElementSpriteSize(getMugenAnimation(getRunnerScreenAnimations(), getBlitzMugenAnimationAnimationNumber(gRunnerBGData.bgEntityId)), getRunnerScreenSprites());

		gRunnerBGData.bgEntityIdSwap = addBlitzEntity(Position(spriteSize.x, 0, 1));
		addBlitzMugenAnimationComponent(gRunnerBGData.bgEntityIdSwap, getRunnerScreenSprites(), getRunnerScreenAnimations(), 1);
		addBlitzPhysicsComponent(gRunnerBGData.bgEntityIdSwap);
		addBlitzPhysicsVelocityX(gRunnerBGData.bgEntityIdSwap, -3.f);

		gRunnerBGData.floorCollisionList = addCollisionListToHandler();
		gRunnerBGData.collisionEntityId = addBlitzEntity(Position(0, 176, 0));
		addBlitzCollisionComponent(gRunnerBGData.collisionEntityId);
		gRunnerBGData.floorCollisionId = addBlitzCollisionRect(gRunnerBGData.collisionEntityId, gRunnerBGData.floorCollisionList, CollisionRect(0, 0, 320, 240 - 176));
		setBlitzCollisionSolid(gRunnerBGData.collisionEntityId, gRunnerBGData.floorCollisionId, 0);
	}

	void updatePositionSwap()
	{
		auto currentLeftPos = getBlitzEntityPositionX(gRunnerBGData.bgEntityId);
		
		if (currentLeftPos <= -spriteSize.x) {
			addBlitzEntityPositionX(gRunnerBGData.bgEntityId, spriteSize.x * 2);
			std::swap(gRunnerBGData.bgEntityId, gRunnerBGData.bgEntityIdSwap);
		}
	
	}

	void update() 
	{
		updatePositionSwap();
	
	}
};

EXPORT_ACTOR_CLASS(RunnerBG)

CollisionListData* getRunnerBGCollisionList()
{
	return gRunnerBGData.floorCollisionList;
}

void setBGSpeed(double s)
{
	setBlitzPhysicsVelocityX(gRunnerBGData.bgEntityId, -s);
	setBlitzPhysicsVelocityX(gRunnerBGData.bgEntityIdSwap, -s);
}