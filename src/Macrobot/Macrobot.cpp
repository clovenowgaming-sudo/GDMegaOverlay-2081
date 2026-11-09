// Macrobot for GD 2.2081 / Geode 5.
// Records GJBaseGameLayer::handleButton with a checkpoint-aware step counter and replays it at the same step.
// RESUME = replay the saved macro up to its save point, then switch to recording.
#include "Macrobot.h"

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/CCScheduler.hpp>

#include <imgui.h>

#include <algorithm>
#include <cctype>
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
	bool frameStep = false;

	namespace
	{
		bool injecting = false;
		int frameOffset = 0;
		bool checkpointLoaded = false;
		int stepsPending = 0;
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

	std::string levelTag()
	{
		std::string name = "level";
		auto pl = PlayLayer::get();
		if (pl && pl->m_level)
			name = std::string(pl->m_level->m_levelName.c_str());
		for (auto& c : name)
			if (!std::isalnum((unsigned char)c))
				c = '_';
		if (name.empty())
			name = "level";
		return name;
	}

	const char* buttonName(int b)
	{
		return b == 1 ? "Jump" : (b == 2 ? "Left" : (b == 3 ? "Right" : "?"));
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

		auto pl = PlayLayer::get();
		if (pl && (playerMode == RECORDING || playerMode == RESUME))
			macro.endFrame = currentFrame();
		if (macro.endFrame <= 0)
			macro.endFrame = macro.inputs.back().frame + 1;
		macro.levelName = levelTag();

		auto now = (long long)std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::string name = macro.levelName;
		if (pl)
			name += "_" + std::to_string((int)pl->getCurrentPercent()) + "pct";
		name += "_" + std::to_string(now) + ".gmo";

		auto path = macroDir() / name;
		std::ofstream f(path);
		if (!f)
		{
			status = "Could not write file";
			return;
		}

		f << "GMO2 " << macro.framerate << ' ' << macro.endFrame << ' ' << macro.levelName << "\n";
		for (auto const& i : macro.inputs)
			f << i.frame << ' ' << i.button << ' ' << (i.down ? 1 : 0) << ' ' << (i.p1 ? 1 : 0) << '\n';

		status = "Saved " + path.filename().string();
		getMacros();
	}

	void exportJson()
	{
		if (macro.inputs.empty())
		{
			status = "Nothing to export";
			return;
		}

		auto now = (long long)std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		auto path = macroDir() / ("macro_" + std::to_string(now) + ".json");
		std::ofstream f(path);
		if (!f)
		{
			status = "Could not write file";
			return;
		}

		f << "{\n  \"fps\": " << macro.framerate << ",\n  \"endFrame\": " << macro.endFrame
		  << ",\n  \"level\": \"" << macro.levelName << "\",\n  \"inputs\": [\n";
		for (size_t i = 0; i < macro.inputs.size(); i++)
		{
			auto const& in = macro.inputs[i];
			f << "    {\"frame\": " << in.frame << ", \"button\": " << in.button
			  << ", \"down\": " << (in.down ? "true" : "false") << ", \"player1\": " << (in.p1 ? "true" : "false") << "}"
			  << (i + 1 < macro.inputs.size() ? "," : "") << "\n";
		}
		f << "  ]\n}\n";

		status = "Exported " + path.filename().string();
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
		int endFrame = 0;
		std::string level;
		f >> magic >> fps;
		if (magic == "GMO2")
			f >> endFrame >> level;
		else if (magic != "GMO1")
		{
			status = "Not a macro file";
			return;
		}

		Macro m;
		m.framerate = fps > 0.f ? fps : 240.f;
		m.endFrame = endFrame;
		m.levelName = level;
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
		if (m.endFrame <= 0 && !m.inputs.empty())
			m.endFrame = m.inputs.back().frame + 1;

		macro = std::move(m);
		actionIndex = 0;
		status = "Loaded " + std::to_string(macro.inputs.size()) + " inputs, ends at frame " + std::to_string(macro.endFrame);
	}

	// after a restart / checkpoint respawn, with currentFrame() already corrected
	void onRespawn()
	{
		int f = currentFrame();

		if (playerMode == RECORDING)
		{
			macro.inputs.erase(
				std::remove_if(macro.inputs.begin(), macro.inputs.end(), [f](Input const& i) { return i.frame >= f; }),
				macro.inputs.end());
		}
		else if (playerMode == PLAYBACK || playerMode == RESUME)
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

	// RESUME reached the save point: the bot stops and the player takes over
	void checkResumeEnd()
	{
		if (playerMode == RESUME && currentFrame() >= macro.endFrame)
		{
			playerMode = RECORDING;
			status = "Recording resumed from frame " + std::to_string(currentFrame());
		}
	}

	bool shouldBlockRealInput()
	{
		return (playerMode == PLAYBACK || playerMode == RESUME) && !injecting && !macro.inputs.empty();
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

	bool consumeStep()
	{
		if (stepsPending > 0)
		{
			stepsPending--;
			return true;
		}
		return false;
	}

	void drawWindow()
	{
		if (ImGui::RadioButton("Disable", playerMode == DISABLED))
			playerMode = DISABLED;

		if (ImGui::RadioButton("Record", playerMode == RECORDING))
		{
			playerMode = RECORDING;
			macro.inputs.clear();
			macro.endFrame = 0;
			macro.framerate = 240.f;
			status = "Recording: restart the level";
		}

		if (ImGui::RadioButton("Play", playerMode == PLAYBACK))
		{
			playerMode = PLAYBACK;
			actionIndex = 0;
			status = "Playback: restart the level";
		}

		if (playerMode == RESUME)
			ImGui::TextWrapped("Resuming: the bot is playing to the save point...");

		ImGui::Separator();
		ImGui::Text("Inputs: %d", (int)macro.inputs.size());
		ImGui::Text("Frame: %d", currentFrame());
		if (macro.endFrame > 0)
			ImGui::Text("Save point: frame %d", macro.endFrame);

		if (ImGui::Button("Save"))
			saveMacro();
		ImGui::SameLine();
		if (ImGui::Button("Clear"))
		{
			macro.inputs.clear();
			macro.endFrame = 0;
			actionIndex = 0;
		}
		ImGui::SameLine();
		if (ImGui::Button("Export JSON"))
			exportJson();

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
		ImGui::SameLine();
		if (ImGui::Button("Continue recording"))
		{
			if (macro.inputs.empty() && macro.endFrame <= 0)
				status = "Load a macro first";
			else
			{
				playerMode = RESUME;
				actionIndex = 0;
				status = "Restart the level: the bot plays to the save point, then you take over";
			}
		}

		ImGui::Separator();
		ImGui::Checkbox("Frame step", &frameStep);
		if (frameStep)
		{
			if (ImGui::Button("Next frame"))
				stepsPending += 1;
			ImGui::SameLine();
			if (ImGui::Button("+10"))
				stepsPending += 10;
			ImGui::SameLine();
			if (ImGui::Button("+60"))
				stepsPending += 60;
		}

		if (ImGui::CollapsingHeader("Editor"))
		{
			static int trimFrame = 0;
			static int shiftFrames = 0;

			ImGui::DragInt("##gmotrim", &trimFrame, 1.f, 0, 100000000);
			ImGui::SameLine();
			if (ImGui::Button("Trim after"))
			{
				int t = trimFrame;
				macro.inputs.erase(
					std::remove_if(macro.inputs.begin(), macro.inputs.end(), [t](Input const& i) { return i.frame > t; }),
					macro.inputs.end());
			}

			ImGui::DragInt("##gmoshift", &shiftFrames, 1.f, -100000, 100000);
			ImGui::SameLine();
			if (ImGui::Button("Shift all"))
			{
				for (auto& i : macro.inputs)
					i.frame = std::max(0, i.frame + shiftFrames);
			}

			int toDelete = -1;
			if (ImGui::BeginChild("##gmoinputs", ImVec2(0, 200), true))
			{
				ImGuiListClipper clipper;
				clipper.Begin((int)macro.inputs.size());
				while (clipper.Step())
					for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
					{
						auto const& in = macro.inputs[i];
						ImGui::PushID(i);
						ImGui::Text("%d: %s %s P%d", in.frame, buttonName(in.button), in.down ? "down" : "up", in.p1 ? 1 : 2);
						ImGui::SameLine();
						if (ImGui::SmallButton("X"))
							toDelete = i;
						ImGui::PopID();
					}
			}
			ImGui::EndChild();

			if (toDelete >= 0 && toDelete < (int)macro.inputs.size())
				macro.inputs.erase(macro.inputs.begin() + toDelete);
		}

		if (!status.empty())
			ImGui::TextWrapped("%s", status.c_str());
	}
}

class $modify(GMOMacroBGL, GJBaseGameLayer)
{
	void processCommands(float dt, bool isHalfTick, bool isLastTick)
	{
		if (Macrobot::isThePlayLayer(this))
		{
			Macrobot::checkResumeEnd();
			if (Macrobot::playerMode == Macrobot::PLAYBACK || Macrobot::playerMode == Macrobot::RESUME)
				Macrobot::injectDue(this);
		}

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
	CheckpointObject* createCheckpoint()
	{
		auto cp = PlayLayer::createCheckpoint();
		if (cp)
			Macrobot::onCheckpointStored(cp);
		return cp;
	}

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

// Frame step: the game only advances when a step was requested
class $modify(GMOStepScheduler, CCScheduler)
{
	void update(float dt)
	{
		if (Macrobot::frameStep && PlayLayer::get())
		{
			if (Macrobot::consumeStep())
				dt = (1.f / 240.f) * 1.001f;
			else
				dt = 0.f;
		}
		CCScheduler::update(dt);
	}
};
