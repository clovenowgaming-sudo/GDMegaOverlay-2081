#include "../Settings.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GameManager.hpp>

using namespace geode::prelude;

class $modify(GMOExtraPlayer, PlayerObject)
{
	void playDeathEffect()
	{
		if (Settings::get<bool>("hack/no_death_effect", false))
			return;
		PlayerObject::playDeathEffect();
	}
};

// client-side only: unlocks icons/colors visually, nothing is sent to the servers
class $modify(GMOExtraGameManager, GameManager)
{
	bool isIconUnlocked(int id, IconType type)
	{
		if (Settings::get<bool>("hack/icon_bypass", false))
			return true;
		return GameManager::isIconUnlocked(id, type);
	}

	bool isColorUnlocked(int key, UnlockType type)
	{
		if (Settings::get<bool>("hack/icon_bypass", false))
			return true;
		return GameManager::isColorUnlocked(key, type);
	}
};
