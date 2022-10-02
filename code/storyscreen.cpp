#include "storyscreen.h"

#include <assert.h>

#include <prism/blitz.h>
#include <prism/stlutil.h>

#include "titlescreen.h"
#include "sfxhandler.h"

using namespace std;

static struct {
	MugenDefScript mScript;
	MugenDefScriptGroup* mCurrentGroup;
	MugenSpriteFile mSprites;

	MugenAnimation* mOldAnimation;
	MugenAnimation* mAnimation;
	MugenAnimationHandlerElement* mAnimationID;
	MugenAnimationHandlerElement* mOldAnimationID;

	Position mOldAnimationBasePosition;
	Position mAnimationBasePosition;

	int mSpeakerID;
	int mTextID;

	int mIsStoryOver;

	char mDefinitionPath[1024];
} gStoryScreenData;

static int isImageGroup() {
	string name = gStoryScreenData.mCurrentGroup->mName;
	char firstW[100];
	sscanf(name.data(), "%s", firstW);

	return !strcmp("image", firstW);
}

static void increaseGroup() {
	gStoryScreenData.mCurrentGroup = gStoryScreenData.mCurrentGroup->mNext;
}

static void loadImageGroup() {
	if (gStoryScreenData.mOldAnimationID != nullptr) {
		removeMugenAnimation(gStoryScreenData.mOldAnimationID);
		destroyMugenAnimation(gStoryScreenData.mOldAnimation);
	}

	if (gStoryScreenData.mAnimationID != nullptr) {
		setMugenAnimationBasePosition(gStoryScreenData.mAnimationID, &gStoryScreenData.mOldAnimationBasePosition);
	}

	gStoryScreenData.mOldAnimationID = gStoryScreenData.mAnimationID;
	gStoryScreenData.mOldAnimation = gStoryScreenData.mAnimation;


	int group = getMugenDefNumberVariableAsGroup(gStoryScreenData.mCurrentGroup, "group");
	int item = getMugenDefNumberVariableAsGroup(gStoryScreenData.mCurrentGroup, "item");
	gStoryScreenData.mAnimation = createOneFrameMugenAnimationForSprite(group, item);
	gStoryScreenData.mAnimationID = addMugenAnimation(gStoryScreenData.mAnimation, &gStoryScreenData.mSprites, Vector3D(0, 0, 0));
	//setMugenAnimationBaseDrawScale(gStoryScreenData.mAnimationID, 2);
	setMugenAnimationBasePosition(gStoryScreenData.mAnimationID, &gStoryScreenData.mAnimationBasePosition);

	increaseGroup();
}


static int isTextGroup() {
	string name = gStoryScreenData.mCurrentGroup->mName;
	char firstW[100];
	sscanf(name.data(), "%s", firstW);

	return !strcmp("text", firstW);
}

static void loadTextGroup() {
	if (gStoryScreenData.mTextID != -1) {
		removeMugenText(gStoryScreenData.mTextID);
		removeMugenText(gStoryScreenData.mSpeakerID);
	}

	char* speaker = getAllocatedMugenDefStringVariableAsGroup(gStoryScreenData.mCurrentGroup, "speaker");
	char* text = getAllocatedMugenDefStringVariableAsGroup(gStoryScreenData.mCurrentGroup, "text");

	gStoryScreenData.mSpeakerID = addMugenText(speaker, Vector3D(70 / 2, 359 / 2, 3), 1);

	gStoryScreenData.mTextID = addMugenText(text, Vector3D(70 / 2, 382 / 2, 3), 1);
	setMugenTextBuildup(gStoryScreenData.mTextID, 1);
	setMugenTextTextBoxWidth(gStoryScreenData.mTextID, 480 / 2);
	//setMugenTextScale(gStoryScreenData.mTextID, 0.75);

	freeMemory(speaker);
	freeMemory(text);

	increaseGroup();
}

static int isTitleGroup() {
	string name = gStoryScreenData.mCurrentGroup->mName;
	char firstW[100];
	sscanf(name.data(), "%s", firstW);

	return !strcmp("title", firstW);
}

static void goToTitle(void* tCaller) {
	(void)tCaller;
	setNewScreen(getTitleScreen());
}

static void loadTitleGroup() {
	gStoryScreenData.mIsStoryOver = 1;

	addFadeOut(30, goToTitle, NULL);
}

static int isGameGroup() {
	string name = gStoryScreenData.mCurrentGroup->mName;
	char firstW[100];
	sscanf(name.data(), "%s", firstW);

	return !strcmp("game", firstW);
}

static void goToGame(void* tCaller) {
	(void)tCaller;
	//setNewScreen(getGameScreen());
}

static void loadGameGroup() {
	gStoryScreenData.mIsStoryOver = 1;

	addFadeOut(30, goToGame, NULL);
}

static void loadNextStoryGroup() {
	int isRunning = 1;
	while (isRunning) {
		if (isImageGroup()) {
			loadImageGroup();
		}
		else if (isTextGroup()) {
			loadTextGroup();
			break;
		}
		else if (isTitleGroup()) {
			loadTitleGroup();
			break;
		}
		else if (isGameGroup()) {
			loadGameGroup();
			break;
		}
		else {
			logError("Unidentified group type.");
			//logErrorString(gStoryScreenData.mCurrentGroup->mName);
			abortSystem();
		}
	}
}

static void findStartOfStoryBoard() {
	gStoryScreenData.mCurrentGroup = gStoryScreenData.mScript.mFirstGroup;

	while (gStoryScreenData.mCurrentGroup && "storystart" != gStoryScreenData.mCurrentGroup->mName) {
		gStoryScreenData.mCurrentGroup = gStoryScreenData.mCurrentGroup->mNext;
	}

	assert(gStoryScreenData.mCurrentGroup);
	gStoryScreenData.mCurrentGroup = gStoryScreenData.mCurrentGroup->mNext;
	assert(gStoryScreenData.mCurrentGroup);

	gStoryScreenData.mAnimationID = nullptr;
	gStoryScreenData.mOldAnimationID = nullptr;
	gStoryScreenData.mTextID = -1;

	gStoryScreenData.mOldAnimationBasePosition = Vector3D(0, 0, 1);
	gStoryScreenData.mAnimationBasePosition = Vector3D(0, 0, 2);

	loadNextStoryGroup();
}



static void loadStoryScreen() {
	instantiateActor(getSFXHandler());

	gStoryScreenData.mIsStoryOver = 0;

	stringstream scriptPath;
	scriptPath << "story/" << gStoryScreenData.mDefinitionPath << ".def";
	loadMugenDefScript(&gStoryScreenData.mScript, scriptPath.str().data());

	scriptPath.str(std::string());
	scriptPath << "story/" << gStoryScreenData.mDefinitionPath << ".sff";
	gStoryScreenData.mSprites = loadMugenSpriteFileWithoutPalette(scriptPath.str());

	findStartOfStoryBoard();

	
	streamMusicFile("tracks/3.ogg");
}


static void updateText() {
	if (gStoryScreenData.mIsStoryOver) return;
	if (gStoryScreenData.mTextID == -1) return;

	if (hasPressedAFlankSingle(0) || hasPressedAFlankSingle(1) || hasPressedStart()) {
		if (isMugenTextBuiltUp(gStoryScreenData.mTextID)) {
			playAverySoundEffect(5);
			loadNextStoryGroup();
		}
		else {
			setMugenTextBuiltUp(gStoryScreenData.mTextID);
		}
	}
}

static void updateStoryScreen() {

	updateText();
}


Screen gStoryScreen;

Screen* getStoryScreen() {
	gStoryScreen = makeScreen(loadStoryScreen, updateStoryScreen);
	return &gStoryScreen;
}

void setCurrentStoryDefinitionFile(const char* tPath) {
	strcpy(gStoryScreenData.mDefinitionPath, tPath);
}
