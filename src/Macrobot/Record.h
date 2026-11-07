#pragma once
// PORT 2.2081: video recorder (ffmpeg + subprocess) is Windows/2.204 only. Stubbed.
#include <imgui.h>

namespace Record
{
	struct StubRecorder { bool m_recording = false; };
	inline StubRecorder recorder;
	inline bool visiting = false;
	inline bool inShader = false;

	inline void drawWindow()
	{
		ImGui::TextWrapped("Video recording is not available in this port.");
	}
}
