// Macrobot for GD 2.2081 / Geode 5.
// Records GJBaseGameLayer::handleButton with a checkpoint-aware step counter and replays it at the same step.
#include "Macrobot.h"

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <imgui.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <unordered_map>

using namespace geode::prelude;

namespace Macrobot
{
	PlayerMode playerMode = DISABLED;
	unsigned int actionIndex = 0;
	Macro macro;

	namespace
	{
		bool injecting = false;
		int frameOffset = 0;
		bool checkpointLoaded = false;
		std::unordered_map<CheckpointObject*, int> checkpointFrames;
		std::vector<std::string> files;
		int selectedFile = -1;
		std::string status;
	}

	std::filesystem::path macroDir()
	{
		auto p = Mod::get()->getSaveDir() / "macros";
		std::error_code ec;
		std::filesystem::create_directories(p, ec);
		return p;
	}

	int rawProgress()
	{
		auto pl = PlayLayer::get();
		if (!pl)
			return 0;
		return static_cast<int>(pl->m_gameState.m_currentProgress);
	}

	// the game's counter + a correction we maintain ourselves, so checkpoint respawns always rewind it
	int currentFrame()
	{
		return rawProgress() + frameOffset;
	}

	void getMacros()
	{
		files.clear();
		std::error_code ec;
		for (auto const& e : std::filesystem::directory_iterator(macroDir(), ec))
			if (e.path().extension() == ".gmo")
				files.push_back(e.path().filename().string());
		std::sort(files.begin(), files.end());
		if (selectedFile >= (int)files.size())
			selectedFile = -1;
	}

	void saveMacro()
	{
		if (macro.inputs.empty())
		{
			status = "Nothing to save";
			return;
		}

		auto now = (long long)std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		auto path = macroDir() / ("macro_" + std::to_string(now) + ".gmo");
		std::ofstream f(path);
		if (!f)
		{
			status = "Could not write file";
			return;
		}

		f << "GMO1 " << macro.framerate << "\n";
		for (auto const& i : macro.inputs)
			f << i.frame << ' ' << i.button << ' ' << (i.down ? 1 : 0) << ' ' << (i.p1 ? 1 : 0) << '\n';

		status = "Saved " + path.filename().string();
		getMacros();
	}

	void loadMacro(std::string const& name)
	{
		std::ifstream f(macroDir() / name);
		if (!f)
		{
			status = "Could not open file";
			return;
		}

		std::string magic;
		float fps = 240.f;
		f >> magic >> fps;
		if (magic != "GMO1")
		{
			status = "Not a macro file";
			return;
		}

		Macro m;
		m.framerate = fps > 0.f ? fps : 240.f;
		int frame, button, down, p1;
		while (f >> frame >> button >> down >> p1)
		{
			Input in;
			in.frame = frame;
			in.button = button;
			in.down = down != 0;
			in.p1 = p1 != 0;
			m.inputs.push_back(in);
		}

		macro = std::move(m);
		actionIndex = 0;
		status = "Loaded " + std::to_string(macro.inputs.size()) + " inputs";
	}

	// after a restart / checkpoint respawn, with currentFrame() already corrected
	void onRespawn()
	{
		int f = currentFrame();

		if (playerMode == RECORDING)
		{
			// everything recorded at or after the respawn frame belongs to the attempt that just died
			macro.inputs.erase(
				std::remove_if(macro.inputs.begin(), macro.inputs.end(), [f](Input const& i) { return i.frame >= f; }),
				macro.inputs.end());
		}
		else if (playerMode == PLAYBACK)
		{
			actionIndex = 0;
			while (actionIndex < macro.inputs.size() && macro.inputs[actionIndex].frame < f)
				actionIndex++;
		}

		if (playerMode != DISABLED)
			log::info("[GMO-Macro] respawn: raw={} offset={} frame={} inputs={}", rawProgress(), frameOffset, f, macro.inputs.size());
	}

	void onCheckpointStored(CheckpointObject* checkpoint)
	{
		checkpointFrames[checkpoint] = currentFrame();
	}

	void onCheckpointLoaded(CheckpointObject* checkpoint)
	{
		auto it = checkpointFrames.find(checkpoint);
		if (it != checkpointFrames.end())
		{
			frameOffset = it->second - rawProgress();
			checkpointLoaded = true;
		}
		onRespawn();
	}

	void onReset()
	{
		// a respawn without a checkpoint is a fresh start: frame 0, whatever the game's counter says
		if (!checkpointLoaded)
			frameOffset = -rawProgress();
		checkpointLoaded = false;
		onRespawn();
	}

	bool isThePlayLayer(GJBaseGameLayer* layer)
	{
		auto pl = PlayLayer::get();
		return pl && static_cast<GJBaseGameLayer*>(pl) == layer;
	}

	void injectDue(GJBaseGameLayer* layer)
	{
		int f = currentFrame();
		injecting = true;
		while (actionIndex < macro.inputs.size() && macro.inputs[actionIndex].frame <= f)
		{
			auto const a = macro.inputs[actionIndex++];
			layer->handleButton(a.down, a.button, a.p1);
		}
		injecting = false;
	}

	bool shouldBlockRealInput()
	{
		return playerMode == PLAYBACK && !injecting && !macro.inputs.empty();
	}

	void record(bool down, int button, bool p1)
	{
		Input in;
		in.frame = currentFrame();
		in.button = button;
		in.down = down;
		in.p1 = p1;
		macro.inputs.push_back(in);
	}

	void drawWindow()
	{
		if (ImGui::RadioButton("Disable", playerMode == DISABLED))
			playerMode = DISABLED;

		if (ImGui::RadioButton("Record", playerMode == RECORDING))
		{
			playerMode = RECORDING;
			macro.inputs.clear();
			macro.framerate = 240.f;
			status = "Recording: restart the level";
		}

		if (ImGui::RadioButton("Play", playerMode == PLAYBACK))
		{
			playerMode = PLAYBACK;
			actionIndex = 0;
			status = "Playback: restart the level";
		}

		ImGui::Separator();
		ImGui::Text("Inputs: %d", (int)macro.inputs.size());
		ImGui::Text("Frame: %d", currentFrame());

		if (ImGui::Button("Save"))
			saveMacro();
		ImGui::SameLine();
		if (ImGui::Button("Clear"))
		{
			macro.inputs.clear();
			actionIndex = 0;
		}

		ImGui::Separator();
		if (ImGui::Button("Refresh list"))
			getMacros();

		const char* preview = (selectedFile >= 0 && selectedFile < (int)files.size()) ? files[selectedFile].c_str() : "Select macro";
		if (ImGui::BeginCombo("##gmomacros", preview))
		{
			for (int i = 0; i < (int)files.size(); i++)
				if (ImGui::Selectable(files[i].c_str(), selectedFile == i))
					selectedFile = i;
			ImGui::EndCombo();
		}

		if (ImGui::Button("Load") && selectedFile >= 0 && selectedFile < (int)files.size())
			loadMacro(files[selectedFile]);

		if (!status.empty())
			ImGui::TextWrapped("%s", status.c_str());
	}
}

class $modify(GMOMacroBGL, GJBaseGameLayer)
{
	void processCommands(float dt, bool isHalfTick, bool isLastTick)
	{
		if (Macrobot::playerMode == Macrobot::PLAYBACK && Macrobot::isThePlayLayer(this))
			Macrobot::injectDue(this);

		GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
	}

	void handleButton(bool down, int button, bool isPlayer1)
	{
		if (Macrobot::isThePlayLayer(this))
		{
			if (Macrobot::shouldBlockRealInput())
				return;

			if (Macrobot::playerMode == Macrobot::RECORDING)
				Macrobot::record(down, button, isPlayer1);
		}

		GJBaseGameLayer::handleButton(down, button, isPlayer1);
	}
};

class $modify(GMOMacroPlayLayer, PlayLayer)
{
	void storeCheckpoint(CheckpointObject* checkpoint)
	{
		PlayLayer::storeCheckpoint(checkpoint);
		Macrobot::onCheckpointStored(checkpoint);
	}

	void loadFromCheckpoint(CheckpointObject* checkpoint)
	{
		PlayLayer::loadFromCheckpoint(checkpoint);
		Macrobot::onCheckpointLoaded(checkpoint);
	}

	void resetLevel()
	{
		PlayLayer::resetLevel();
		Macrobot::onReset();
	}
};
