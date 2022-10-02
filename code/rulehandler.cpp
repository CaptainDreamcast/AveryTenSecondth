#include "rulehandler.h"
#include "storyscreen.h"

#include "runnerplayer.h"
#include "runnerbg.h"
#include "platformmanager.h"
#define RULE_TEXT_Z 20

enum class RuleType : int {
	EMPTY = 0,
	SIZE,
	SPEED,
	GRAVITY,
	DAMAGE,
	UP,
	DIRECTION,
	SOUND,
	MYSTERY,
	VISION,
	JOKE,
	FINAL
};

struct Rule {
	std::string s1;
	std::string s2;
	RuleType id;
};

void startFinale() { setBGSpeed(0.f); setPlatformSpeed(0.f); }

class RuleHandler
{
public:
	std::vector<Rule> mRules;
	int currentIndex = -1;

	int mCounterTextID;
	int mBanterTextID;


	RuleHandler() 
	{
		mCounterTextID = addMugenTextMugenStyle("Time to rule change: 10 seconds", Position(3, 12, RULE_TEXT_Z), Vector3DI(1, 0, 1));
		mBanterTextID = addMugenTextMugenStyle("G.O.D.: BANTER BANTER BANTER BANTER BANTER BANTER BANTER BANTER BANTER", Position(3, 190, RULE_TEXT_Z), Vector3DI(1, 0, 1));
		setMugenTextTextBoxWidth(mBanterTextID, 280);

		loadRules();
		gotoNextRule();
	}

	void loadRules()
	{
		MugenDefScript script;
		loadMugenDefScript(&script, "runner/RULES.def");

		MugenDefScriptGroup* group = script.mFirstGroup;
		while (group) {
			loadRuleFromGroup(group);
			group = group->mNext;
		}
	}

	void loadRuleFromGroup(MugenDefScriptGroup* group) {
		Rule r;
		r.s1 = getSTLMugenDefStringOrDefaultAsGroup(group, "s1", "");
		r.s2 = getSTLMugenDefStringOrDefaultAsGroup(group, "s2", "");
		r.id = RuleType(mRules.size());
		mRules.push_back(r);
	}

	int isInSecondHalf = 0;
	void gotoNextRule()
	{
		if(currentIndex >= 0) deactivateActiveRule();
		currentIndex++;
		if (currentIndex >= mRules.size()) {
			setCurrentStoryDefinitionFile("OUTRO");
			setNewScreen(getStoryScreen());
			return;
		}

		isInSecondHalf = 0;
		executeActiveRule();
		setFirstRuleTextActive();
	}

	void update() 
	{
		updateCountdown();
	}

	void executeActiveRule() {
		auto r = mRules[currentIndex];
		if (r.id == RuleType::EMPTY) {}
		if (r.id == RuleType::MYSTERY) {}
		if (r.id == RuleType::JOKE) {}
		if (r.id == RuleType::SIZE) { setPlayerSize(0.25f); }
		if (r.id == RuleType::SPEED) { setBGSpeed(6.f); setPlatformSpeed(6.f);}
		if (r.id == RuleType::GRAVITY) { setPlayerGravity(0.05f); }
		if (r.id == RuleType::DAMAGE) { setPlayerInvincible(true); }
		if (r.id == RuleType::UP) { setPlayerGravity(-0.001f); }
		if (r.id == RuleType::DIRECTION) { reversePlayerDirection(true); }
		if (r.id == RuleType::SOUND) { setVolume(0.0); }
		if (r.id == RuleType::VISION) { setPlayerBlind(); }
		if (r.id == RuleType::FINAL) { startFinale(); }

	}
	void deactivateActiveRule() {
		auto r = mRules[currentIndex];
		if (r.id == RuleType::EMPTY) {}
		if (r.id == RuleType::MYSTERY) { setRandomSeed(unsigned(this)); }
		if (r.id == RuleType::JOKE) {}
		if (r.id == RuleType::SIZE) { setPlayerSize(1.f); }
		if (r.id == RuleType::SPEED) { setBGSpeed(3.f); setPlatformSpeed(3.f); }
		if (r.id == RuleType::GRAVITY) { setPlayerGravity(0.1f); }
		if (r.id == RuleType::DAMAGE) { setPlayerInvincible(false); }
		if (r.id == RuleType::UP) { setPlayerGravity(0.1f); }
		if (r.id == RuleType::DIRECTION) { reversePlayerDirection(false); }
		if (r.id == RuleType::SOUND) { setVolume(1.0); }
		if (r.id == RuleType::VISION) { setPlayerUnblind(); }
	}
	
	void setFirstRuleTextActive() {
		changeMugenText(mBanterTextID, mRules[currentIndex].s1.c_str());
		setMugenTextBuildup(mBanterTextID, 1);
	}
	void setSecondRuleTextActive() {
		changeMugenText(mBanterTextID, mRules[currentIndex].s2.c_str());
		setMugenTextBuildup(mBanterTextID, 1);
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

		if (timeLeft < 5.f && !isInSecondHalf) {
			setSecondRuleTextActive();
			isInSecondHalf = 1;
		}

		timeLeft -= 1.0 / getFramerate();
		if (timeLeft <= 0)
		{
			gotoNextRule();
			timeLeft = 9.99f;

		}
	}
};

EXPORT_ACTOR_CLASS(RuleHandler);