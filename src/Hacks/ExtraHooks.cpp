#include "../Settings.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/HardStreak.hpp>
#include <Geode/modify/GhostTrailEffect.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

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

class $modify(GMOExtraStreak, HardStreak)
{
	void updateStroke(float dt)
	{
		if (Settings::get<bool>("hack/solid_wave", false))
			m_isSolid = true;
		HardStreak::updateStroke(dt);
	}
};

class $modify(GMOExtraGhost, GhostTrailEffect)
{
	void trailSnapshot(float dt)
	{
		if (Settings::get<bool>("hack/no_ghost_trail", false))
			return;
		GhostTrailEffect::trailSnapshot(dt);
	}
};

class $modify(GMOExtraBGL, GJBaseGameLayer)
{
	void toggleFlipped(bool flipped, bool animated)
	{
		if (Settings::get<bool>("hack/no_mirror_portal", false))
			return;
		GJBaseGameLayer::toggleFlipped(flipped, animated);
	}
};

// Show Hitboxes: uses the game's own debug draw, driven by the existing menu settings
static bool s_gmoHitboxesForced = false;

class $modify(GMOExtraPlayLayer, PlayLayer)
{
	void postUpdate(float dt)
	{
		PlayLayer::postUpdate(dt);

		bool enabled = Settings::get<bool>("level/show_hitbox/enabled", false);
		bool onDeath = Settings::get<bool>("level/show_hitbox/on_death", false);
		bool dead = m_player1 && m_player1->m_isDead;
		bool want = enabled && (!onDeath || dead);

		if (want)
		{
			m_isDebugDrawEnabled = true;
			if (m_debugDrawNode)
				m_debugDrawNode->setVisible(true);
			this->updateDebugDraw();
			s_gmoHitboxesForced = true;
		}
		else if (s_gmoHitboxesForced)
		{
			m_isDebugDrawEnabled = false;
			if (m_debugDrawNode)
			{
				m_debugDrawNode->clear();
				m_debugDrawNode->setVisible(false);
			}
			s_gmoHitboxesForced = false;
		}
	}
};
