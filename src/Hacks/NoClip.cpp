#include "../Settings.hpp"
#include "../Common.h"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(GMONoClipPlayLayer, PlayLayer)
{
	void destroyPlayer(PlayerObject* player, GameObject* object)
	{
		if (Settings::get<bool>("patch/NoClip", false))
		{
			// keep the anti-cheat spike behaviour of the game untouched
			if (object != m_anticheatSpike)
				return;
		}

		PlayLayer::destroyPlayer(player, object);
	}
};
