#include "../Settings.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GameObject.hpp>
#include <Geode/modify/GameStatsManager.hpp>

using namespace geode::prelude;

static bool s_gmoInstantDone = false;
static bool s_gmoLayoutMode = false;

class $modify(GMOBypassPlayLayer, PlayLayer)
{
	void resetLevel()
	{
		PlayLayer::resetLevel();
		s_gmoInstantDone = false;
	}

	void postUpdate(float dt)
	{
		PlayLayer::postUpdate(dt);

		// cached once per frame so GameObject::setVisible stays cheap
		s_gmoLayoutMode = Settings::get<bool>("level/layout_mode", false);

		// Safe Mode is forced on while this is enabled, so nothing gets saved or submitted
		if (Settings::get<bool>("level/instant_complete", false) && !s_gmoInstantDone && m_player1 && !m_player1->m_isDead)
		{
			s_gmoInstantDone = true;
			this->levelComplete();
		}
	}
};

// Layout Mode: decorations are never shown
class $modify(GMOBypassObject, GameObject)
{
	void setVisible(bool visible)
	{
		if (visible && s_gmoLayoutMode && m_objectType == GameObjectType::Decoration && PlayLayer::get())
			visible = false;
		GameObject::setVisible(visible);
	}
};

// Main Levels: every main level counts as unlocked
class $modify(GMOBypassStats, GameStatsManager)
{
	bool hasCompletedMainLevel(int levelID)
	{
		if (Settings::get<bool>("hack/main_levels", false))
			return true;
		return GameStatsManager::hasCompletedMainLevel(levelID);
	}
};
