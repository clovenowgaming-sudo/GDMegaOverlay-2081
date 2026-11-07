#include "PhysicsBypass.h"
#include "Settings.hpp"
#include "Macrobot/Macrobot.h"
#include <Geode/modify/PlayLayer.hpp>
#include "Common.h"

class $modify(PlayLayer)
{
	bool init(GJGameLevel* p0, bool p1, bool p2)
	{
		PhysicsBypass::timestamp = p0->m_timestamp;

		return PlayLayer::init(p0, p1, p2);
	}

	void resetLevel()
	{
		PlayLayer::resetLevel();
		PhysicsBypass::calculateTickrate();
	}
};

void PhysicsBypass::calculateTickrate()
{
	// PORT-TODO 2.2081: the 2.204 version wrote 1/tps to a hardcoded global (base+0x49D548) and
	// rescaled m_gameState.m_unk1f8. The physics step is handled differently in 2.2081, so the TPS
	// bypass needs a fresh hook (see NOTES). Left as a no-op so nothing gets corrupted.
}
