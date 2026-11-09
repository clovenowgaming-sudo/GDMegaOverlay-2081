#pragma once

namespace Record
{
	struct Recorder
	{
		bool m_recording = false;
	};

	extern Recorder recorder;
	extern bool visiting;
	extern bool inShader;

	bool isRecording();
	int currentFps();
	void captureFrame();
	void drawWindow();

	void onLevelStart();
	void onLevelComplete();
	void onLevelExit();
}
