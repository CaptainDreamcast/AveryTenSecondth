#include "runnerscreen.h"
#include "runnerbg.h"
#include "runnerplayer.h"
#include "platformmanager.h"
#include "rulehandler.h"
#include "sfxhandler.h"

static struct {
	MugenSpriteFile mSprites;
	MugenAnimations mAnimations;

	int mRunnerBGId;
	int mRunnerID;
	int mPlatformManagerID;
	int mRuleHandlerID;
} gRunnerScreenData;

class RunnerScreen
{
public:
	RunnerScreen() {
		instantiateActor(getSFXHandler());

		gRunnerScreenData.mSprites = loadMugenSpriteFileWithoutPalette("runner/RUNNER.sff");
		gRunnerScreenData.mAnimations = loadMugenAnimationFile("runner/RUNNER.air");

		gRunnerScreenData.mRunnerBGId = instantiateActor(getRunnerBG());
		gRunnerScreenData.mPlatformManagerID = instantiateActor(getPlatformManager());
		gRunnerScreenData.mRunnerID = instantiateActor(getRunnerPlayer());
		gRunnerScreenData.mRuleHandlerID = instantiateActor(getRuleHandler());


		addCollisionHandlerCheck(getRunnerPlayerPlatformCollisionList(), getRunnerBGCollisionList());
		addCollisionHandlerCheck(getRunnerPlayerEnemyCollisionList(), getEnemyCollisionList());
		//activateCollisionHandlerDebugMode();

		streamMusicFile("tracks/4.ogg");
	}

	void update() {
		return;

		if (hasPressedR())
		{
			setWrapperTimeDilatation(10.f);
		}
		else
		{
			setWrapperTimeDilatation(1.f);
		}
	}

};

EXPORT_SCREEN_CLASS(RunnerScreen);

MugenSpriteFile* getRunnerScreenSprites()
{
	return &gRunnerScreenData.mSprites;
}

MugenAnimations* getRunnerScreenAnimations()
{
	return &gRunnerScreenData.mAnimations;
}

int getRunningScreenScale()
{
	return 4;
}

void loadRunnerScreenSpritesPlusAnimationsSolo()
{
	gRunnerScreenData.mSprites = loadMugenSpriteFileWithoutPalette("runner/RUNNER.sff");
	gRunnerScreenData.mAnimations = loadMugenAnimationFile("runner/RUNNER.air");
}