#pragma once
// PORT 2.2081: Macrobot needs PlayerObject/PlayLayer checkpoint layouts that were hardcoded
// by offset for 2.204. The original is kept in legacy_2204/ . This stub keeps the rest of the
// overlay compiling and shows a notice in the Macrobot window.
#include <imgui.h>
#include <vector>
#include <string>

namespace Macrobot
{
	enum PlayerMode { DISABLED = -1, PLAYBACK = 0, RECORDING = 1 };

	struct StubMacro
	{
		float framerate = 240.f;
		std::vector<int> inputs;
	};

	inline PlayerMode playerMode = DISABLED;
	inline unsigned int actionIndex = 0;
	inline StubMacro macro;

	inline void getMacros() {}
	inline void drawWindow()
	{
		ImGui::TextWrapped("Macrobot is not ported to GD 2.2081 yet.");
	}
}
