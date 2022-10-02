#include "sfxhandler.h"

static struct
{
	MugenSounds mSounds;

}gSFXHandlerData;

class SFXHandler {
public:
	SFXHandler() {
		gSFXHandlerData.mSounds = loadMugenSoundFile("runner/GAME.snd");
	}
	void update() {}

};

EXPORT_ACTOR_CLASS(SFXHandler)

void playAverySoundEffect(int id)
{
	playMugenSound(&gSFXHandlerData.mSounds, 1, id);
}
