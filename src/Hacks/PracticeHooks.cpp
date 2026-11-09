#include "../Settings.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

static float s_gmoCheckpointTimer = 0.f;

class $modify(GMOPracticePlayLayer, PlayLayer)
{
	void resetLevel()
	{
		PlayLayer::resetLevel();
		s_gmoCheckpointTimer = 0.f;
	}

	void postUpdate(float dt)
	{
		PlayLayer::postUpdate(dt);

		// Instant Respawn: the game reads the respawn delay from the game state
		if (Settings::get<bool>("practice/instant_respawn", false))
			m_gameState.m_respawnTime = Settings::get<float>("practice/respawn_time", 0.f);

		// Auto Checkpoint: practice mode only
		if (Settings::get<bool>("practice/auto_checkpoint", false) && m_isPracticeMode && m_player1 && !m_player1->m_isDead)
		{
			s_gmoCheckpointTimer += dt;
			if (s_gmoCheckpointTimer >= Settings::get<float>("practice/checkpoint_interval", 2.f))
			{
				s_gmoCheckpointTimer = 0.f;
				this->markCheckpoint();
			}
		}
	}
};
