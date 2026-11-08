// Internal video recorder using the FFmpeg API mod (eclipse.ffmpeg-api).
// While recording, the game is stepped with a fixed delta (1/fps): every rendered frame is one video frame.
#include "Record.h"

#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/CCScheduler.hpp>

#include <eclipse.ffmpeg-api/include/events.hpp>

#include <imgui.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

using namespace geode::prelude;

namespace Record
{
	Recorder recorder;
	bool visiting = false;
	bool inShader = false;

	namespace
	{
		std::unique_ptr<ffmpeg::events::Recorder> encoder;
		std::vector<std::string> codecs;
		bool codecsLoaded = false;
		int codecIndex = 0;
		int fpsIndex = 0;
		int bitrateMbps = 10;
		int scaleIndex = 1;
		int srcW = 0, srcH = 0, outW = 0, outH = 0;
		unsigned int frames = 0;
		std::string status;
		std::string lastFile;
		std::vector<uint8_t> readBuffer;
		std::vector<uint8_t> frameBuffer;

		const int fpsOptions[] = {30, 60, 120};
		const char* fpsNames[] = {"30", "60", "120"};
		const float scaleOptions[] = {1.f, 0.75f, 0.5f};
		const char* scaleNames[] = {"100%", "75%", "50%"};
	}

	bool isRecording() { return recorder.m_recording; }
	int currentFps() { return fpsOptions[fpsIndex]; }

	static void loadCodecs()
	{
		if (codecsLoaded)
			return;
		codecsLoaded = true;

		ffmpeg::events::Recorder probe;
		codecs = probe.getAvailableCodecs();

		for (size_t i = 0; i < codecs.size(); i++)
			if (codecs[i] == "libx264")
			{
				codecIndex = (int)i;
				break;
			}
	}

	static void startRecording()
	{
		loadCodecs();
		if (codecs.empty())
		{
			status = "FFmpeg API returned no codecs (is the mod installed?)";
			return;
		}

		auto fs = CCDirector::get()->getOpenGLView()->getFrameSize();
		srcW = (int)fs.width;
		srcH = (int)fs.height;
		float scale = scaleOptions[scaleIndex];
		outW = ((int)(srcW * scale)) & ~1;
		outH = ((int)(srcH * scale)) & ~1;

		std::error_code ec;
		auto dir = Mod::get()->getSaveDir() / "videos";
		std::filesystem::create_directories(dir, ec);

		std::string codec = codecs[codecIndex];
		auto now = (long long)std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		auto path = dir / ("gmo_" + std::to_string(now) + (codec == "libvpx" ? ".webm" : ".mp4"));

		ffmpeg::RenderSettings settings;
		settings.m_pixelFormat = ffmpeg::PixelFormat::RGB0;
		settings.m_codec = codec;
		settings.m_bitrate = bitrateMbps * 1000000;
		settings.m_width = outW;
		settings.m_height = outH;
		settings.m_fps = fpsOptions[fpsIndex];
		settings.m_outputFile = path.string();

		encoder = std::make_unique<ffmpeg::events::Recorder>();

		bool ok = true;
		using InitResult = decltype(encoder->init(settings));
		if constexpr (std::is_void_v<InitResult>)
			encoder->init(settings);
		else
		{
			auto r = encoder->init(settings);
			ok = r.isOk();
		}

		if (!ok)
		{
			encoder.reset();
			status = "Could not start the encoder with codec " + codec;
			return;
		}

		frames = 0;
		lastFile = path.string();
		recorder.m_recording = true;
		status = "Recording...";
	}

	static void stopRecording()
	{
		recorder.m_recording = false;
		if (encoder)
		{
			encoder->stop();
			encoder.reset();
		}
		status = "Saved " + std::to_string(frames) + " frames";
	}

	void captureFrame()
	{
		if (!encoder || srcW <= 0 || srcH <= 0)
			return;

		readBuffer.resize((size_t)srcW * srcH * 4);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, srcW, srcH, GL_RGBA, GL_UNSIGNED_BYTE, readBuffer.data());

		// flip vertically and scale (nearest) to the output size
		frameBuffer.resize((size_t)outW * outH * 4);
		for (int y = 0; y < outH; y++)
		{
			int sy = (outH - 1 - y) * srcH / outH;
			const uint8_t* srow = readBuffer.data() + (size_t)sy * srcW * 4;
			uint8_t* drow = frameBuffer.data() + (size_t)y * outW * 4;
			for (int x = 0; x < outW; x++)
				std::memcpy(drow + (size_t)x * 4, srow + (size_t)(x * srcW / outW) * 4, 4);
		}

		encoder->writeFrame(frameBuffer);
		frames++;
	}

	void drawWindow()
	{
		loadCodecs();

		if (recorder.m_recording)
		{
			ImGui::Text("Recording: %u frames (%.1fs)", frames, frames / (float)fpsOptions[fpsIndex]);
			if (ImGui::Button("Stop recording"))
				stopRecording();
		}
		else
		{
			if (codecs.empty())
				ImGui::TextWrapped("No codecs found. The FFmpeg API mod must be installed.");
			else
			{
				const char* preview = (codecIndex >= 0 && codecIndex < (int)codecs.size()) ? codecs[codecIndex].c_str() : "-";
				if (ImGui::BeginCombo("Codec", preview))
				{
					for (int i = 0; i < (int)codecs.size(); i++)
						if (ImGui::Selectable(codecs[i].c_str(), codecIndex == i))
							codecIndex = i;
					ImGui::EndCombo();
				}

				if (ImGui::BeginCombo("FPS", fpsNames[fpsIndex]))
				{
					for (int i = 0; i < 3; i++)
						if (ImGui::Selectable(fpsNames[i], fpsIndex == i))
							fpsIndex = i;
					ImGui::EndCombo();
				}

				if (ImGui::BeginCombo("Scale", scaleNames[scaleIndex]))
				{
					for (int i = 0; i < 3; i++)
						if (ImGui::Selectable(scaleNames[i], scaleIndex == i))
							scaleIndex = i;
					ImGui::EndCombo();
				}

				ImGui::DragInt("Bitrate (Mbps)", &bitrateMbps, 0.2f, 1, 100);

				if (ImGui::Button("Start recording"))
					startRecording();
			}
		}

		if (!status.empty())
			ImGui::TextWrapped("%s", status.c_str());
		if (!lastFile.empty())
			ImGui::TextWrapped("File: %s", lastFile.c_str());
	}
}

// fixed timestep while recording, so every rendered frame is exactly one video frame
class $modify(GMORecordScheduler, CCScheduler)
{
	void update(float dt)
	{
		if (Record::isRecording())
			dt = 1.f / (float)Record::currentFps();
		CCScheduler::update(dt);
	}
};

// capture the finished frame (overlay included) right after the scene is drawn
class $modify(GMORecordDirector, CCDirector)
{
	void drawScene()
	{
		CCDirector::drawScene();
		if (Record::isRecording())
			Record::captureFrame();
	}
};
