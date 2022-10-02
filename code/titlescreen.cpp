#include "titlescreen.h"
#include "runnerbg.h"
#include "runnerscreen.h"

#include "runnerscreen.h"
#include "sfxhandler.h"

void goToGame(void*)
{
	setNewScreen(getRunnerScreen());
}

class TitleScreen {
public:
	MugenSpriteFile mSprites;
	MugenAnimations mAnimations;
	int pressStartEntity;
	int mCounterTextID;
	TitleScreen() {
		instantiateActor(getSFXHandler());


		loadRunnerScreenSpritesPlusAnimationsSolo();
		instantiateActor(getRunnerBG());

		mSprites = loadMugenSpriteFileWithoutPalette("title/TITLE.sff");
		mAnimations = loadMugenAnimationFile("title/TITLE.air");

		int entityId = addBlitzEntity(Position(160, 80, 20));
		addBlitzMugenAnimationComponent(entityId, &mSprites, &mAnimations, 1);

		pressStartEntity = addBlitzEntity(Position(160, 140, 20));
		addBlitzMugenAnimationComponent(pressStartEntity, &mSprites, &mAnimations, 100);

		mCounterTextID = addMugenTextMugenStyle("Time to rule change: 10 seconds", Position(3, 12, 20), Vector3DI(1, 0, 1));

		currentIndex = 0;
		setWrapperTitleScreen(getTitleScreen());
		streamMusicFile("tracks/4.ogg");
	}

	int currentIndex = 0;
	int amount = 4;
	int (*func[4])() = { hasPressedAFlank , hasPressedBFlank, hasPressedXFlank, hasPressedYFlank};
	int anims[4] = { 100, 101, 102, 103 };

	void updatePressDisplay() {
		currentIndex = randfromInteger(0, amount - 1);
		changeBlitzMugenAnimation(pressStartEntity, anims[currentIndex]);
	}

	void updateHasPressed() {
		if (func[currentIndex]())
		{
			addFadeOut(20, goToGame);
		}
	
	}

	double timeLeft = 9.99f;
	void updateCountdown()
	{
		std::stringstream ss;
		ss << "Time to rule change: " << int(timeLeft) + 1;
		if (int(timeLeft) == 0) {
			ss << " second";
		}
		else
		{
			ss << " seconds";
		}
		changeMugenText(mCounterTextID, ss.str().c_str());

		timeLeft -= 1.0 / getFramerate();
		if (timeLeft <= 0)
		{
			timeLeft = 9.99f;
			updatePressDisplay();
		}
	}

	void update() {
		updateCountdown();
		updateHasPressed();
	}

};

EXPORT_SCREEN_CLASS(TitleScreen)