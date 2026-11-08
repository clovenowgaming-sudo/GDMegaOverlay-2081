#include "../Settings.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/GameLevelManager.hpp>
#include <Geode/modify/SetupTriggerPopup.hpp>
#include <Geode/modify/CreatorLayer.hpp>

using namespace geode::prelude;

// client-side request cooldown only
class $modify(GMOBypassLevelManager, GameLevelManager)
{
	bool isTimeValid(char const* key, float time)
	{
		if (Settings::get<bool>("hack/timeout_bypass", false))
			return true;
		return GameLevelManager::isTimeValid(key, time);
	}
};

// trigger popups no longer clamp slider values
class $modify(GMOBypassTriggerPopup, SetupTriggerPopup)
{
	bool shouldLimitValue(int tag)
	{
		if (Settings::get<bool>("hack/slider_limit", false))
			return false;
		return SetupTriggerPopup::shouldLimitValue(tag);
	}
};

// Vault / Treasure Room: open the scene directly, skipping the entry check
class $modify(GMOBypassCreator, CreatorLayer)
{
	void onSecretVault(CCObject* sender)
	{
		if (Settings::get<bool>("hack/vault_bypass", false))
		{
			CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, SecretLayer2::scene()));
			return;
		}
		CreatorLayer::onSecretVault(sender);
	}

	void onTreasureRoom(CCObject* sender)
	{
		if (Settings::get<bool>("hack/treasure_room_bypass", false))
		{
			CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, SecretRewardsLayer::scene(false)));
			return;
		}
		CreatorLayer::onTreasureRoom(sender);
	}
};
