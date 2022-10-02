#include "runnerplayer.h"

#include "runnerscreen.h"
#include "sfxhandler.h"

static struct
{
	CollisionListData* runnerPlatformCollisionList;
	CollisionListData* runnerEnemyCollisionList;

} gRunnerData;

class RunnerPlayer
{
public:
	int mEntityId;
	int mCollisionIdPlatform;
	int mCollisionIdEnemy;
	int mStartPositionX = 20;
	bool isDead = false;
	int specialInvincibility = 0;
	int inverseDirection = 0;

	int mLifeAmount = 100;
	int mLifeDisplayId;
	int mLifeNumberId;
	RunnerPlayer() {


		gRunnerData.runnerPlatformCollisionList = addCollisionListToHandler();
		gRunnerData.runnerEnemyCollisionList = addCollisionListToHandler();

		mEntityId = addBlitzEntity(Position(mStartPositionX, 175 - 50, 10));
		addBlitzMugenAnimationComponent(mEntityId, getRunnerScreenSprites(), getRunnerScreenAnimations(), 10);
		addBlitzCollisionComponent(mEntityId);
		mCollisionIdPlatform = addBlitzCollisionRect(mEntityId, gRunnerData.runnerPlatformCollisionList, CollisionRect(-12, -14, 24, 15));
		setBlitzCollisionSolid(mEntityId, mCollisionIdPlatform, 1);
		mCollisionIdEnemy = addBlitzCollisionRect(mEntityId, gRunnerData.runnerEnemyCollisionList, CollisionRect(-7, -9, 15, 10));
		addBlitzPhysicsComponent(mEntityId);
		setBlitzPhysicsGravity(mEntityId, Acceleration(0, 0.1, 0));
		setBlitzMugenAnimationTransparency(mEntityId, 0.6f);

		mLifeDisplayId = addBlitzEntity(Position(290, 238, 19));
		addBlitzMugenAnimationComponent(mLifeDisplayId, getRunnerScreenSprites(), getRunnerScreenAnimations(), 300);
		mLifeNumberId = addMugenTextMugenStyle("100", Position(292, 230, 19), Vector3DI(1, 0, 1));
	}



	void update() {
		updateJumping();
		updatePosition();
		updateAccelerate();
		updateBlind();
		updateInvincibility();
	}

	void updateAccelerate() {
		if (!accelerateLeft) return;
		accelerateLeft--;

		if (!accelerateLeft) {
			setBlitzPhysicsGravity(mEntityId, Acceleration(0, 0.1, 0));
		}
	}

	double veloc = 3.f;
#
	int isInvincible = 120;
	void resetPlayerToAlive() {
		if (!mLifeAmount) { setNewScreen(getRunnerScreen()); return; }

		playAverySoundEffect(3);

		mLifeAmount--;
		stringstream ss;
		ss << mLifeAmount;
		changeMugenText(mLifeNumberId, ss.str().c_str());

		isInvincible = 120;
		isDead = 0;
		changeBlitzMugenAnimation(mEntityId, 10);
		setBlitzMugenAnimationTransparency(mEntityId, 0.6f);
		setBlitzEntityPosition(mEntityId, Position(mStartPositionX, 175 - 50, 10));
		setBlitzPhysicsVelocity(mEntityId, Position(0, 0, 0));
	}

	void updateInvincibility()
	{
		if (!isInvincible) return;
		isInvincible--;
		if (!isInvincible) {
			setBlitzMugenAnimationTransparency(mEntityId, 1.f);
		}
	}

	void updatePosition() {
		if (getBlitzEntityPositionX(mEntityId) < -20 || getBlitzEntityPositionX(mEntityId) > 340) {
			resetPlayerToAlive();
		}
		if (isDead) { setBlitzPhysicsVelocityX(mEntityId, -veloc); return; }
		if ((specialInvincibility || isInvincible) && getBlitzEntityPositionX(mEntityId) < 0) { setBlitzEntityPositionX(mEntityId, 0); }

		if ((hasPressedLeft() && !inverseDirection) || (hasPressedRight() && inverseDirection)) {
			addBlitzEntityPositionX(mEntityId, -veloc);
		}
		else if ((hasPressedRight() && !inverseDirection) || (hasPressedLeft() && inverseDirection)) {
			addBlitzEntityPositionX(mEntityId, veloc);
		}
	}

	void updateJumping() {
		updateJumpEnd();
		updateJumpStart();
	}

	bool isJumping = false;
	void updateJumpStart() {
		if (isJumping) return;
		if (isDead) return;
		
		if (hasPressedAFlank())
		{
			setBlitzPhysicsVelocity(mEntityId, Velocity(0, 0, 0));
			addBlitzPhysicsImpulse(mEntityId, Acceleration(0, -4, 0));
			playAverySoundEffect(0);
			isJumping = true;
		}
	}

	void updateJumpEnd() {
		if (!isJumping) return;

		auto velocity = getBlitzPhysicsVelocity(mEntityId);
		if (hasBlitzCollidedBottom(mEntityId) && abs(velocity.y) < 1e-4)
		{
			playAverySoundEffect(1);
			isJumping = false;
		}
	}

	void hitBySpike()
	{
		if (isDead || specialInvincibility || isInvincible) return;

		playAverySoundEffect(4);
		isDead = true;

		changeBlitzMugenAnimation(mEntityId, 8000);	
	}

	void hitByBounce()
	{
		setBlitzPhysicsVelocity(mEntityId, Velocity(0, 0, 0));
		addBlitzPhysicsImpulse(mEntityId, Acceleration(0, -4, 0));
		playAverySoundEffect(2);
		isJumping = true;
	}

	int accelerateLeft = 0;
	void hitByAccelerate()
	{
		playAverySoundEffect(3);
		accelerateLeft = 60 * 1;
		setBlitzPhysicsGravity(mEntityId, Acceleration(0, 0, 0));
		setBlitzPhysicsVelocity(mEntityId, Velocity(0, 0, 0));
	}

	int isBlind = 0;
	int blindEntity;
	void setPlayerBlind()
	{
		auto pos = (getBlitzEntityPosition(mEntityId));
		pos.z = 11;
		blindEntity = addBlitzEntity(pos);
		addBlitzMugenAnimationComponent(blindEntity, getRunnerScreenSprites(), getRunnerScreenAnimations(), 200);
		isBlind = 1;
	}
	void setPlayerUnblind()
	{
		removeBlitzEntity(blindEntity);
		isBlind = 0;
	}

	void updateBlind() {
		if (!isBlind) return;
		auto pos = (getBlitzEntityPosition(mEntityId));
		pos.z = 11;
		setBlitzEntityPosition(blindEntity, pos);;
	}
};

EXPORT_ACTOR_CLASS(RunnerPlayer);

void setPlayerHitBySpike()
{
	gRunnerPlayer->hitBySpike();
}

void setPlayerHitByBounce()
{
	gRunnerPlayer->hitByBounce();
}

void setPlayerHitByAccelerate()
{
	gRunnerPlayer->hitByAccelerate();
}

void setPlayerSize(double s)
{
	setBlitzEntityScale2D(gRunnerPlayer->mEntityId, s);
}

void setPlayerGravity(double s)
{
	setBlitzPhysicsGravity(gRunnerPlayer->mEntityId, Vector3D(0, s, 0));
}

void setPlayerInvincible(int isInv)
{
	gRunnerPlayer->specialInvincibility = isInv;
}

void reversePlayerDirection(int isRev)
{
	gRunnerPlayer->inverseDirection = isRev;
}

void setPlayerBlind()
{
	gRunnerPlayer->setPlayerBlind();
}
void setPlayerUnblind()
{
	gRunnerPlayer->setPlayerUnblind();
}

CollisionListData* getRunnerPlayerPlatformCollisionList()
{
	return gRunnerData.runnerPlatformCollisionList;
}

CollisionListData* getRunnerPlayerEnemyCollisionList()
{
	return gRunnerData.runnerEnemyCollisionList;
}
