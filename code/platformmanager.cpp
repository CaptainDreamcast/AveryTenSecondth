#include "platformmanager.h"
#include "runnerscreen.h"
#include "runnerbg.h"
#include "runnerplayer.h"

#define PLATFORM_Z 10
#define TILE_SIZE 23
static struct {
	CollisionListData* mEnemyCollisionList;
	int mDistanceEntityId;
} gPlatformManagerData;

struct Platform
{
	int mEntityId;
	int mCollisionId;
};

struct Spike
{
	int mEntityId;
	int mCollisionId;
};

struct Bounce
{
	int mEntityId;
	int mCollisionId;
	int mWasUsed;
};

struct Accelerate
{
	int mEntityId;
	int mCollisionId;
	int mWasUsed;
};

enum class PlacementType
{
	PLACEMENT_TYPE_PLATFORM,
	PLACEMENT_TYPE_SPIKE,
	PLACEMENT_TYPE_BOUNCE,
	PLACEMENT_TYPE_ACCELERATE,
};

struct PlacementEntry
{
	PlacementType type;
	int x; // in tiles
	double y; // in tiles
	int mirrorX;
	int mirrorY;
};

struct PlacementGroup
{
	int placementGroupSizeInTiles;
	std::vector<PlacementEntry> orderedEntries;
};

void spikeHitCB(void*, void*);
void bounceHitCB(void*, void*);
void accelerateHitCB(void*, void*);

class PlatformManager
{
public:
	std::list<Platform> mPlatforms;
	std::list<Spike> mSpikes;
	std::list<Bounce> mBounces;
	std::list<Accelerate> mAccelerates;
	std::vector<PlacementGroup> mLoadedPlacementGroups;

	PlatformManager() {
		gPlatformManagerData.mEnemyCollisionList = addCollisionListToHandler();
		gPlatformManagerData.mDistanceEntityId = addBlitzEntity(Position(0, 0, 0));
		addBlitzPhysicsComponent(gPlatformManagerData.mDistanceEntityId);
		setBlitzPhysicsVelocityX(gPlatformManagerData.mDistanceEntityId, 1.f);
		loadGroupPresetsFromFile();
	}

	void loadGroupPresetsFromFile()
	{
		MugenDefScript presetScript;
		loadMugenDefScript(&presetScript, "runner/PRESETS.def");

		MugenDefScriptGroup* currentGroup = presetScript.mFirstGroup;
		while (currentGroup)
		{
			if (isPresetStartGroup(currentGroup))
			{
				loadPreset(currentGroup);
			}
			else
			{
				currentGroup = currentGroup->mNext;
			}
		}
	}

	int isPresetStartGroup(MugenDefScriptGroup* currentGroup)
	{
		return currentGroup->mName == "presetstart";
	}

	void loadPreset(MugenDefScriptGroup*& currentGroup)
	{	
		std::multimap<int, PlacementEntry> entryMap;
		currentGroup = currentGroup->mNext;
		while (currentGroup && isPlacementGroup(currentGroup))
		{
			PlacementEntry entry;
			entry.x = getMugenDefIntegerOrDefaultAsGroup(currentGroup, "x", 0);
			entry.y = getMugenDefFloatOrDefaultAsGroup(currentGroup, "y", 0);
			entry.mirrorX = getMugenDefIntegerOrDefaultAsGroup(currentGroup, "mirrorx", 0);
			entry.mirrorY = getMugenDefIntegerOrDefaultAsGroup(currentGroup, "mirrory", 0);
			auto typeString = getSTLMugenDefStringOrDefaultAsGroup(currentGroup, "type", "platform");
			entry.type = placementTypeFromString(typeString);
			entryMap.insert(std::make_pair(entry.x, entry));

			currentGroup = currentGroup->mNext;
		}

		mLoadedPlacementGroups.emplace_back();
		auto& placementGroup = mLoadedPlacementGroups.back();
		placementGroup.placementGroupSizeInTiles = 0;
		for (auto& placementEntry : entryMap)
		{
			placementGroup.orderedEntries.push_back(placementEntry.second);
			placementGroup.placementGroupSizeInTiles = std::max(placementGroup.placementGroupSizeInTiles, placementEntry.second.x);
		}
	}

	PlacementType placementTypeFromString(const std::string& name)
	{
		if (name == "platform")
		{
			return PlacementType::PLACEMENT_TYPE_PLATFORM;
		}
		else if (name == "spike")
		{
			return PlacementType::PLACEMENT_TYPE_SPIKE;
		}
		else if (name == "bounce")
		{
			return PlacementType::PLACEMENT_TYPE_BOUNCE;
		}
		else if (name == "accelerate")
		{
			return PlacementType::PLACEMENT_TYPE_ACCELERATE;
		}

		logWarningFormat("[PlatformManager] Unable to parse placement type: %s. Defaulting to platform.", name.c_str());
		return PlacementType::PLACEMENT_TYPE_PLATFORM;
	}

	int isPlacementGroup(MugenDefScriptGroup* currentGroup)
	{
		return currentGroup->mName == "placement";
	}

	void update() {
		updatePlacement();
		updatePlatforms();
		updateSpikes();
		updateBounces();
		updateAccelerates();
	}

	void updatePlacement() {
		updatePlacementGroupOverAndUpdate();
		updatePlacementGroupSelection();
	}

	PlacementGroup* mCurrentPlacementGroup = NULL;
	int currentPlacementGroupIndex = 0;
	int mLastPlacedEntityId = -1;
	void updatePlacementGroupOverAndUpdate()
	{
		if (!mCurrentPlacementGroup) return;

		bool hasNextPlacementItemToCheck = true;
		auto groupPlacementOffset = getNextGroupPlacementOffset();
		while (hasNextPlacementItemToCheck) {
			if (currentPlacementGroupIndex >= int(mCurrentPlacementGroup->orderedEntries.size()))
			{
				mCurrentPlacementGroup = NULL;
				hasNextPlacementItemToCheck = false;
			}
			else
			{
				auto next = mCurrentPlacementGroup->orderedEntries[currentPlacementGroupIndex];
				auto nextPlacementX = next.x * TILE_SIZE + groupPlacementOffset;
				if (nextPlacementX > 360.f && currentPlacementGroupIndex == 0)
				{
					hasNextPlacementItemToCheck = false;
				}
				else
				{
					createPlacementFromEntry(next, groupPlacementOffset);
					currentPlacementGroupIndex++;
				}
			
			}
		}
	}

	double getNextGroupPlacementOffset()
	{
		if (mLastPlacedEntityId == -1) return 360.f;
		return getBlitzEntityPositionX(mLastPlacedEntityId) + TILE_SIZE * 3;
	}

	void createPlacementFromEntry(const PlacementEntry& entry, double groupPlacementOffset)
	{
		auto x = entry.x * TILE_SIZE + groupPlacementOffset;
		auto y = 176 - TILE_SIZE - entry.y * TILE_SIZE;
		Position2D pos = Position2D(x, y);
		if (entry.type == PlacementType::PLACEMENT_TYPE_PLATFORM)
		{
			createPlatform(pos, entry.mirrorX, entry.mirrorY);
		}
		else if (entry.type == PlacementType::PLACEMENT_TYPE_SPIKE)
		{
			createSpike(pos, entry.mirrorX, entry.mirrorY);
		}
		else if (entry.type == PlacementType::PLACEMENT_TYPE_BOUNCE)
		{
			createBounce(pos, entry.mirrorX, entry.mirrorY);
		}
		else if (entry.type == PlacementType::PLACEMENT_TYPE_ACCELERATE)
		{
			createAccelerate(pos, entry.mirrorX, entry.mirrorY);
		}
		else
		{
			logWarningFormat("[PlatformManager] Unable to parse entry type to create: %d", int(entry.type));
			return;
		}
	}

	void updatePlacementGroupSelection()
	{
		if (mCurrentPlacementGroup) return;

		int index = randfromInteger(0, mLoadedPlacementGroups.size() - 1);
		mCurrentPlacementGroup = &mLoadedPlacementGroups[index];
		currentPlacementGroupIndex = 0;
	}

	void updatePlatforms()
	{
		updatePlatformRemoval();
	}
	void updatePlatformRemoval()
	{
		bool hasPlatformsToCheck = true;
		while (hasPlatformsToCheck)
		{
			hasPlatformsToCheck = checkIfOldestPlatformShouldBeRemovedAndRemove();
		}
	}

	bool checkIfOldestPlatformShouldBeRemovedAndRemove() {
		if (mPlatforms.empty()) return false;
	
		auto& platform = mPlatforms.front();
		auto posX = getBlitzEntityPositionX(platform.mEntityId);
		if (posX > -60)
		{
			return false;
		}
		removeBlitzEntity(platform.mEntityId);
		mPlatforms.pop_front();
		return true;
	}

	float velocity = -3.f;
	void createPlatform(const Position2D& pos, bool isMirroredX, bool isMirroredY) 
	{
		mPlatforms.emplace_back();
		auto& platform = mPlatforms.back();
		platform.mEntityId = addBlitzEntity(Position(pos.x, pos.y, PLATFORM_Z));
		addBlitzMugenAnimationComponent(platform.mEntityId, getRunnerScreenSprites(), getRunnerScreenAnimations(), 100);
		if(isMirroredY) setBlitzMugenAnimationPositionY(platform.mEntityId, TILE_SIZE);
		setBlitzMugenAnimationFaceDirection(platform.mEntityId, !isMirroredX);
		setBlitzMugenAnimationVerticalFaceDirection(platform.mEntityId, !isMirroredY);
		addBlitzCollisionComponent(platform.mEntityId);
		platform.mCollisionId = addBlitzCollisionRect(platform.mEntityId, getRunnerBGCollisionList(), CollisionRect(0, 0, 23, 23));
		setBlitzCollisionSolid(platform.mEntityId, platform.mCollisionId, 0);
		addBlitzPhysicsComponent(platform.mEntityId);
		setBlitzPhysicsVelocityX(platform.mEntityId, velocity);
		mLastPlacedEntityId = platform.mEntityId;
	}

	void updateSpikes()
	{
		updateSpikeRemoval();
	}
	void updateSpikeRemoval()
	{
		bool hasSpikesToCheck = true;
		while (hasSpikesToCheck)
		{
			hasSpikesToCheck = checkIfOldestSpikeShouldBeRemovedAndRemove();
		}
	}

	bool checkIfOldestSpikeShouldBeRemovedAndRemove() {
		if (mSpikes.empty()) return false;

		auto& spike = mSpikes.front();
		auto posX = getBlitzEntityPositionX(spike.mEntityId);
		if (posX > -60)
		{
			return false;
		}
		removeBlitzEntity(spike.mEntityId);
		mSpikes.pop_front();
		return true;
	}

	void createSpike(const Position2D& pos, bool isMirroredX, bool isMirroredY)
	{
		mSpikes.emplace_back();
		auto& spike = mSpikes.back();
		spike.mEntityId = addBlitzEntity(Position(pos.x, pos.y, PLATFORM_Z));
		addBlitzMugenAnimationComponent(spike.mEntityId, getRunnerScreenSprites(), getRunnerScreenAnimations(), 101);
		setBlitzMugenAnimationFaceDirection(spike.mEntityId, !isMirroredX);
		setBlitzMugenAnimationVerticalFaceDirection(spike.mEntityId, !isMirroredY);
		if (isMirroredY) setBlitzMugenAnimationPositionY(spike.mEntityId, TILE_SIZE);

		addBlitzCollisionComponent(spike.mEntityId);
		spike.mCollisionId = addBlitzCollisionRect(spike.mEntityId, getRunnerBGCollisionList(), CollisionRect(5, 7, 13, 16));
		addBlitzPhysicsComponent(spike.mEntityId);
		setBlitzPhysicsVelocityX(spike.mEntityId, velocity);
		addBlitzCollisionCB(spike.mEntityId, spike.mCollisionId, spikeHitCB, NULL);
		mLastPlacedEntityId = spike.mEntityId;
	}

	void updateBounces()
	{
		updateBounceRemoval();
	}
	void updateBounceRemoval()
	{
		bool hasBouncesToCheck = true;
		while (hasBouncesToCheck)
		{
			hasBouncesToCheck = checkIfOldestBounceShouldBeRemovedAndRemove();
		}
	}

	bool checkIfOldestBounceShouldBeRemovedAndRemove() {
		if (mBounces.empty()) return false;

		auto& bounce = mBounces.front();
		auto posX = getBlitzEntityPositionX(bounce.mEntityId);
		if (posX > -60)
		{
			return false;
		}
		removeBlitzEntity(bounce.mEntityId);
		mBounces.pop_front();
		return true;
	}

	void createBounce(const Position2D& pos, bool isMirroredX, bool isMirroredY)
	{
		mBounces.emplace_back();
		auto& bounce = mBounces.back();
		bounce.mEntityId = addBlitzEntity(Position(pos.x, pos.y, PLATFORM_Z));
		addBlitzMugenAnimationComponent(bounce.mEntityId, getRunnerScreenSprites(), getRunnerScreenAnimations(), 102);
		setBlitzMugenAnimationFaceDirection(bounce.mEntityId, !isMirroredX);
		setBlitzMugenAnimationVerticalFaceDirection(bounce.mEntityId, !isMirroredY);
		if (isMirroredY) setBlitzMugenAnimationPositionY(bounce.mEntityId, TILE_SIZE);

		addBlitzCollisionComponent(bounce.mEntityId);
		bounce.mCollisionId = addBlitzCollisionRect(bounce.mEntityId, getRunnerBGCollisionList(), CollisionRect(5, 7, 13, 16));
		addBlitzPhysicsComponent(bounce.mEntityId);
		setBlitzPhysicsVelocityX(bounce.mEntityId, velocity);
		addBlitzCollisionCB(bounce.mEntityId, bounce.mCollisionId, bounceHitCB, &bounce);
		mLastPlacedEntityId = bounce.mEntityId;
	}


	void updateAccelerates()
	{
		updateAccelerateRemoval();
	}
	void updateAccelerateRemoval()
	{
		bool hasAcceleratesToCheck = true;
		while (hasAcceleratesToCheck)
		{
			hasAcceleratesToCheck = checkIfOldestAccelerateShouldBeRemovedAndRemove();
		}
	}

	bool checkIfOldestAccelerateShouldBeRemovedAndRemove() {
		if (mAccelerates.empty()) return false;

		auto& accelerate = mAccelerates.front();
		auto posX = getBlitzEntityPositionX(accelerate.mEntityId);
		if (posX > -60)
		{
			return false;
		}
		removeBlitzEntity(accelerate.mEntityId);
		mAccelerates.pop_front();
		return true;
	}

	void createAccelerate(const Position2D& pos, bool isMirroredX, bool isMirroredY)
	{
		mAccelerates.emplace_back();
		auto& accelerate = mAccelerates.back();
		accelerate.mEntityId = addBlitzEntity(Position(pos.x, pos.y, PLATFORM_Z));
		addBlitzMugenAnimationComponent(accelerate.mEntityId, getRunnerScreenSprites(), getRunnerScreenAnimations(), 103);
		setBlitzMugenAnimationFaceDirection(accelerate.mEntityId, !isMirroredX);
		setBlitzMugenAnimationVerticalFaceDirection(accelerate.mEntityId, !isMirroredY);
		if (isMirroredY) setBlitzMugenAnimationPositionY(accelerate.mEntityId, TILE_SIZE);

		addBlitzCollisionComponent(accelerate.mEntityId);
		accelerate.mCollisionId = addBlitzCollisionRect(accelerate.mEntityId, getRunnerBGCollisionList(), CollisionRect(5, 7, 13, 16));
		addBlitzPhysicsComponent(accelerate.mEntityId);
		setBlitzPhysicsVelocityX(accelerate.mEntityId, velocity);
		addBlitzCollisionCB(accelerate.mEntityId, accelerate.mCollisionId, accelerateHitCB, &accelerate);
		mLastPlacedEntityId = accelerate.mEntityId;
	}

	void setPlatformSpeed(double s)
	{
		velocity = -s;
		for (auto s : mPlatforms) { setBlitzPhysicsVelocityX(s.mEntityId, velocity); }
		for (auto s : mSpikes) { setBlitzPhysicsVelocityX(s.mEntityId, velocity); }
		for (auto s : mBounces) { setBlitzPhysicsVelocityX(s.mEntityId, velocity); }
		for (auto s : mAccelerates) { setBlitzPhysicsVelocityX(s.mEntityId, velocity); }
	}
};

EXPORT_ACTOR_CLASS(PlatformManager);

CollisionListData* getEnemyCollisionList()
{
	return gPlatformManagerData.mEnemyCollisionList;
}

void spikeHitCB(void*, void*)
{
	setPlayerHitBySpike();
}

void bounceHitCB(void* tCaller, void*)
{
	Bounce* bounce = (Bounce*)tCaller;
	if (bounce->mWasUsed) 
	{
		return;
	}
	bounce->mWasUsed = true;
	setPlayerHitByBounce();
}

void accelerateHitCB(void* tCaller, void*)
{
	Accelerate* accelerate = (Accelerate*)tCaller;
	if (accelerate->mWasUsed)
	{
		return;
	}
	accelerate->mWasUsed = true;
	setPlayerHitByAccelerate();
}

void setPlatformSpeed(double s)
{
	gPlatformManager->setPlatformSpeed(s);

}