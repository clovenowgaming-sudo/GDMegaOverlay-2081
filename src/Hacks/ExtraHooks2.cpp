#include "../Settings.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(GMOMorePlayer, PlayerObject)
{
	void playSpawnEffect()
	{
		if (Settings::get<bool>("hack/no_spawn_effect", false))
			return;
		PlayerObject::playSpawnEffect();
	}

	void playSpiderDashEffect(CCPoint from, CCPoint to)
	{
		if (Settings::get<bool>("hack/no_spider_dash", false))
			return;
		PlayerObject::playSpiderDashEffect(from, to);
	}

	void playBurstEffect()
	{
		if (Settings::get<bool>("hack/no_burst_effect", false))
			return;
		PlayerObject::playBurstEffect();
	}
};

class $modify(GMOMoreBGL, GJBaseGameLayer)
{
	void lightningFlash(CCPoint from, CCPoint to, ccColor3B color, float lineWidth, float duration, int displacement, bool flash, float opacity)
	{
		if (Settings::get<bool>("hack/no_lightning", false))
			return;
		GJBaseGameLayer::lightningFlash(from, to, color, lineWidth, duration, displacement, flash, opacity);
	}

	void shakeCamera(float duration, float strength, float interval)
	{
		if (Settings::get<bool>("hack/no_shake", false))
			return;
		GJBaseGameLayer::shakeCamera(duration, strength, interval);
	}

	void updateZoom(float zoom, float duration, int easing, float rate, int uniqueID, int controlID)
	{
		if (Settings::get<bool>("hack/no_zoom", false))
			return;
		GJBaseGameLayer::updateZoom(zoom, duration, easing, rate, uniqueID, controlID);
	}
};

class $modify(GMOMorePlayLayer, PlayLayer)
{
	void playEndAnimationToPos(CCPoint position)
	{
		if (Settings::get<bool>("hack/no_end_anim", false))
		{
			this->showCompleteText();
			return;
		}
		PlayLayer::playEndAnimationToPos(position);
	}
};
