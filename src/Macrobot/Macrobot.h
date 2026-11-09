#pragma once
#include <string>
#include <vector>

namespace Macrobot
{
	enum PlayerMode { DISABLED = -1, PLAYBACK = 0, RECORDING = 1, RESUME = 2 };

	struct Input
	{
		int frame = 0;
		int button = 1;
		bool down = false;
		bool p1 = true;
	};

	struct Macro
	{
		float framerate = 240.f;
		int endFrame = 0;
		std::string levelName;
		std::vector<Input> inputs;
	};

	extern PlayerMode playerMode;
	extern unsigned int actionIndex;
	extern Macro macro;
	extern bool frameStep;

	void getMacros();
	void drawWindow();
}
