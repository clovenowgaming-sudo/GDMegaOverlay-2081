#include "GUI.h"
#include <Geode/Geode.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/MenuLayer.hpp>

#include "WindowAction.h"
#include "../Settings.hpp"

#include <fstream>
#include <sstream>

#include <imgui-cocos.hpp>
#include "ConstData.h"

#include "Common.h"
#include "Hacks/SafeMode.h"

#include "Blur.h"
#include "../AndroidButton.hpp"

using namespace geode::prelude;

class $modify(CCKeyboardDispatcher)
{
	bool dispatchKeyboardMSG(enumKeyCodes key, bool down, bool arr, double timestamp)
	{
		if (!arr)
		{
			int menuKey = Settings::get<int>("menu/togglekey", ImGuiKey_Tab);

			if (menuKey == 9 /* VK_TAB */)
				menuKey = ImGuiKey_Tab;

			if (down && (ConvertKeyEnum(key) == menuKey))
			{
				GUI::toggle();
				return true;
			}

			if (down)
			{
				for (GUI::Shortcut& s : GUI::shortcuts)
				{
					if (ConvertKeyEnum(key) == s.key)
					{
						GUI::currentShortcut = s.name;
						GUI::shortcutLoop = true;
						GUI::draw();
						GUI::shortcutLoop = false;

						Common::updateCheating();
						SafeMode::updateAuto();
					}
				}
			}

			if (!ImGuiCocos::get().isInitialized())
				return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, arr, timestamp);

			if (const auto imKey = ConvertKeyEnum(key); imKey != ImGuiKey_None)
				ImGui::GetIO().AddKeyEvent(imKey, down);
		}

		return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down, arr, timestamp);
	}
};

class $modify(MenuLayer)
{
	bool init()
	{
		if (!GUI::hasLateInit)
		{
			GUI::lateInit();
			GUI::canToggle = true;
		}
		GUI::hasLateInit = true;

		#ifdef GEODE_IS_ANDROID
		AndroidButton::ensureCreated();
		#endif

		return MenuLayer::init();
	}
};

ImVec2 GUI::getJsonPosition(const std::string& name)
{
	if (!windowPositions.contains(name))
	{
		windowPositions[name]["x"] = .0f;
		windowPositions[name]["y"] = .0f;
	}

	auto winSize = CCDirector::sharedDirector()->getOpenGLView()->getViewPortRect();

	if (windowPositions[name]["x"] > 1 || windowPositions[name]["y"] > 1)
		winSize.size = cocos2d::CCSize(1.f, 1.f);

	return {windowPositions[name]["x"].get<float>() * winSize.size.width, windowPositions[name]["y"].get<float>() * winSize.size.height};
}

void GUI::setJsonPosition(const std::string& name, ImVec2 pos)
{
	windowPositions[name]["x"] = pos.x;
	windowPositions[name]["y"] = pos.y;
}

ImVec2 GUI::getJsonSize(const std::string& name, ImVec2 defaultSize)
{
	if (!windowPositions.contains(name) || !windowPositions[name].contains("w"))
	{
		windowPositions[name]["w"] = defaultSize.x;
		windowPositions[name]["h"] = defaultSize.y;
	}

	auto winSize = CCDirector::sharedDirector()->getOpenGLView()->getViewPortRect();

	if (windowPositions[name]["w"] > 1 || windowPositions[name]["h"] > 1)
		winSize.size = cocos2d::CCSize(1.f, 1.f);

	return {windowPositions[name]["w"].get<float>() * winSize.size.width, windowPositions[name]["h"].get<float>() * winSize.size.height};
}

void GUI::setJsonSize(const std::string& name, ImVec2 size)
{
	windowPositions[name]["w"] = size.x;
	windowPositions[name]["h"] = size.y;
}

void GUI::init()
{
	hasLateInit = false;
	uiSizeFactor = CCDirector::sharedDirector()->getOpenGLView()->getFrameSize().width / 1920.0f;
	Blur::compiled = false;
	
	windowPositions = json::object();

	std::filesystem::path defaultStylePath = Mod::get()->getSaveDir() / "styles" / "default.style";
	std::filesystem::copy(Mod::get()->getResourcesDir() / "Style.style", defaultStylePath, std::filesystem::copy_options::overwrite_existing);

	std::filesystem::path defaultFontPath = Mod::get()->getSaveDir() / "fonts" / "Roboto-Regular.ttf";
	std::filesystem::copy(Mod::get()->getResourcesDir() / "Roboto-Regular.ttf", defaultFontPath, std::filesystem::copy_options::overwrite_existing);

	styleCombo = DirectoryCombo("Style", Mod::get()->getSaveDir() / "styles", "menu/style_path");
	fontCombo = DirectoryCombo("Font", Mod::get()->getSaveDir() / "fonts", "menu/font_path");

	std::string fontPath = Settings::get<std::string>("menu/font_path", std::string(defaultFontPath.string()));
	
	if(fontPath.empty() || !std::filesystem::exists(fontPath))
		fontPath = std::string(defaultFontPath.string());

	std::string stylePath = Settings::get<std::string>("menu/style_path", std::string(defaultStylePath.string()));
	if(stylePath.empty() || !std::filesystem::exists(stylePath))
		stylePath = std::string(defaultStylePath.string());
	
	GUI::loadStyle(stylePath);
	setFont(fontPath);

	shortcuts.clear();
	shadowTexture = cocos2d::CCTextureCache::get()->addImage(std::string((Mod::get()->getResourcesDir() / "shadow.png").string()).c_str(), true);
	load();
}

void GUI::setLateInit(const std::function<void()>& func)
{
	lateInit = func;
}

void GUI::draw()
{
	uiSizeFactor = CCDirector::sharedDirector()->getOpenGLView()->getFrameSize().width / 1920.0f;
	uiSizeFactor *= Settings::get<float>("menu/ui_scale", 1.f);

	if (!isVisible && !shortcutLoop)
		return;

	ImGui::GetStyle() = loadedStyle;

	ImGui::GetStyle().ScaleAllSizes(uiSizeFactor);

	for (WindowAction* ac : windowActions)
		ac->step(ImGui::GetIO().DeltaTime);

	for (Window& w : windows)
	{
		if(w.enabled)
			w.draw();
	}

	float transitionDuration = Settings::get<float>("menu/transition_duration", 0.35f);

	hideTimer += ImGui::GetIO().DeltaTime;
	if (hideTimer > transitionDuration)
		isVisible = false;
}

void GUI::toggle()
{
	if (!canToggle)
		return;

	Common::updateCheating();
	SafeMode::updateAuto();
	Blur::compileBlurShader();

	isVisible = true;
	toggled = !toggled;
																											//isPaused
#ifdef GEODE_IS_DESKTOP
	{
		auto scene = CCDirector::get()->getRunningScene();
		bool paused = scene && scene->getChildByType<PauseLayer>(0);
		cocos2d::CCEGLView::sharedOpenGLView()->showCursor(toggled || !PlayLayer::get() || paused || (PlayLayer::get() && PlayLayer::get()->getChildByID("EndLevelLayer")));
	}
#endif

	// 🔥🔥🔥🔥
	hideTimer = toggled ? -FLT_MAX : 0;

	if (!toggled)
		save();

	for (WindowAction* ac : windowActions)
		delete ac;

	windowActions.clear();

	int direction = std::rand() % 8 + 1;

	for (Window& w : windows)
	{
		uint8_t animationType = (w.name.length() + direction) % 8;

		ImVec2 dir;

		switch (animationType)
		{
		case 0:
			dir = {1400, 1400};
			break;
		case 1:
			dir = {1400, -1400};
			break;
		case 2:
			dir = {-1400, 1400};
			break;
		case 3:
			dir = {-1400, -1400};
			break;
		case 4:
			dir = {-2400, 0};
			break;
		case 5:
			dir = {2400, 0};
			break;
		case 6:
			dir = {0, -2400};
			break;
		case 7:
			dir = {0, 2400};
			break;
		}

		float transitionDuration = Settings::get<float>("menu/transition_duration", 0.35f);

		WindowAction* action = WindowAction::create(
			transitionDuration, &w, toggled ? w.position : ImVec2(w.position.x + dir.x, w.position.y + dir.y));
		windowActions.push_back(action);
	}
}

void GUI::addWindow(Window window)
{
	if (windowPositions.contains(window.name))
	{
		window.position = getJsonPosition(window.name);
		window.size = getJsonSize(window.name, window.size);
	}

	int direction = std::rand() % 4 + 1;

	uint8_t animationType = (window.name.length() + direction) % 4;

	switch (animationType)
	{
	case 0:
		window.renderPosition = {1400, 1400};
		break;
	case 1:
		window.renderPosition = {1400, -1400};
		break;
	case 2:
		window.renderPosition = {-1400, 1400};
		break;
	case 3:
		window.renderPosition = {-1400, -1400};
		break;
	}

	windows.push_back(window);
	windowReferences[window.name] = &windows.back();
}

void GUI::save()
{
	for (Window& w : windows)
	{
		windowPositions[w.name]["x"] = (float)w.position.x / (float)ImGui::GetIO().DisplaySize.x;
		windowPositions[w.name]["y"] = (float)w.position.y / (float)ImGui::GetIO().DisplaySize.y;
		windowPositions[w.name]["w"] = (float)w.size.x / (float)ImGui::GetIO().DisplaySize.x;
		windowPositions[w.name]["h"] = (float)w.size.y / (float)ImGui::GetIO().DisplaySize.y;
	}

	std::ofstream f(Mod::get()->getSaveDir() / "windows.json");
	f << windowPositions.dump(4);
	f.close();

	f.open(Mod::get()->getSaveDir() / "shortcuts.json");
	json shortcutArray = json::array();

	for (Shortcut& s : shortcuts)
	{
		json shortcutObject = json::object();
		shortcutObject["name"] = s.name;
		shortcutObject["key"] = s.key;
		shortcutArray.push_back(shortcutObject);
	}

	f << shortcutArray.dump(4);
	f.close();

	Mod::get()->saveData();
}

void GUI::load()
{
	std::ifstream f(Mod::get()->getSaveDir() / "windows.json");

	if (!f)
	{
		f.close();

		f.open(Mod::get()->getResourcesDir() / "default_windows.json");
	}

	if (f)
	{
		std::stringstream buffer;
		buffer << f.rdbuf();
		try
		{
			windowPositions = json::parse(buffer.str());
		}
		catch(json::parse_error& ex)
		{
			log::error("{}", ex.what());
			windowPositions = json::object();
		}
		buffer.clear();
	}
	f.close();

	f.open(Mod::get()->getSaveDir() / "shortcuts.json");
	if (f)
	{
		std::stringstream buffer;
		buffer << f.rdbuf();
		json shortcutArray = json::object();
		try
		{
			shortcutArray = json::parse(buffer.str());
		}
		catch(json::parse_error& ex)
		{
			log::error("{}", ex.what());
			shortcutArray = json::object();
		}
		buffer.clear();

		for (json shortcutObject : shortcutArray)
		{
			Shortcut s(shortcutObject["key"], shortcutObject["name"]);
			GUI::shortcuts.push_back(s);
		}
	}
	f.close();
}

void GUI::resetDefault()
{
	std::ifstream f(Mod::get()->getResourcesDir() / "default_windows.json");

	if (f)
	{
		std::stringstream buffer;
		buffer << f.rdbuf();
		try
		{
			windowPositions = json::parse(buffer.str());
		}
		catch(json::parse_error& ex)
		{
			log::error("{}", ex.what());
			windowPositions = json::object();
		}
		buffer.clear();
	}
	f.close();

	for(Window& window : windows)
	{
		window.position = getJsonPosition(window.name);
		window.renderPosition = window.position;
		window.size = getJsonSize(window.name, window.size);
	}
}

void GUI::saveStyle(const std::filesystem::path& name)
{
	std::ofstream styleFile(name, std::ios::binary);
	if (styleFile)
	{
		styleFile.write((const char*)&loadedStyle, sizeof(ImGuiStyle));
		styleFile.write((const char*)&fontScale, sizeof(float));
	}
	styleFile.close();
}

#include <fstream>
#include <vector>
#include <iterator>
#include <cstring>

// GMO-DEFAULT-STYLE-BEGIN
// Original GDMegaOverlay look (values extracted from the shipped Style.style).
static void gmoFillDefaultStyle(ImGuiStyle& s)
{
	s.Alpha = 1.0f;
	s.DisabledAlpha = 0.5f;
	s.WindowPadding = ImVec2(15.0f, 15.0f);
	s.WindowRounding = 0.0f;
	s.WindowBorderSize = 0.0f;
	s.WindowMinSize = ImVec2(32.0f, 32.0f);
	s.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	s.WindowMenuButtonPosition = (ImGuiDir)0;
	s.ChildRounding = 0.0f;
	s.ChildBorderSize = 1.0f;
	s.PopupRounding = 0.0f;
	s.PopupBorderSize = 1.0f;
	s.FramePadding = ImVec2(5.0f, 4.0f);
	s.FrameRounding = 0.0f;
	s.FrameBorderSize = 0.0f;
	s.ItemSpacing = ImVec2(12.0f, 7.0f);
	s.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
	s.CellPadding = ImVec2(4.0f, 2.0f);
	s.TouchExtraPadding = ImVec2(0.0f, 0.0f);
	s.IndentSpacing = 25.0f;
	s.ColumnsMinSpacing = 6.0f;
	s.ScrollbarSize = 10.0f;
	s.ScrollbarRounding = 9.0f;
	s.GrabMinSize = 5.0f;
	s.GrabRounding = 0.0f;
	s.LogSliderDeadzone = 4.0f;
	s.TabRounding = 4.0f;
	s.TabBorderSize = 0.0f;
	s.TabMinWidthForCloseButton = 0.0f;
	s.ColorButtonPosition = (ImGuiDir)1;
	s.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	s.SelectableTextAlign = ImVec2(0.0f, 0.0f);
	s.DisplayWindowPadding = ImVec2(19.0f, 19.0f);
	s.DisplaySafeAreaPadding = ImVec2(3.0f, 3.0f);
	s.MouseCursorScale = 1.0f;
	s.AntiAliasedLines = true;
	s.AntiAliasedLinesUseTex = true;
	s.AntiAliasedFill = true;
	s.CurveTessellationTol = 1.25f;
	s.CircleTessellationMaxError = 1.6f;
	s.Colors[0] = ImVec4(0.8f, 0.8f, 0.83f, 1.0f);
	s.Colors[1] = ImVec4(1.0f, 0.03f, 0.03f, 1.0f);
	s.Colors[2] = ImVec4(0.09417f, 0.094169f, 0.094169f, 1.0f);
	s.Colors[3] = ImVec4(0.07f, 0.07f, 0.09f, 1.0f);
	s.Colors[4] = ImVec4(0.06f, 0.05f, 0.07f, 1.0f);
	s.Colors[5] = ImVec4(0.43f, 0.43f, 0.5f, 0.5f);
	s.Colors[6] = ImVec4(0.92f, 0.91f, 0.88f, 0.0f);
	s.Colors[7] = ImVec4(0.14f, 0.15f, 0.18f, 0.3f);
	s.Colors[8] = ImVec4(0.14f, 0.15f, 0.18f, 0.4f);
	s.Colors[9] = ImVec4(0.14f, 0.15f, 0.18f, 0.5f);
	s.Colors[10] = ImVec4(0.685531f, 0.171672f, 0.717647f, 1.0f);
	s.Colors[11] = ImVec4(0.685531f, 0.171672f, 0.717647f, 1.0f);
	s.Colors[12] = ImVec4(0.685531f, 0.171672f, 0.717647f, 1.0f);
	s.Colors[13] = ImVec4(0.1f, 0.09f, 0.12f, 1.0f);
	s.Colors[14] = ImVec4(0.1f, 0.09f, 0.12f, 1.0f);
	s.Colors[15] = ImVec4(0.8f, 0.8f, 0.83f, 0.31f);
	s.Colors[16] = ImVec4(0.56f, 0.56f, 0.58f, 1.0f);
	s.Colors[17] = ImVec4(0.06f, 0.05f, 0.07f, 1.0f);
	s.Colors[18] = ImVec4(0.0f, 0.8f, 0.0f, 0.611765f);
	s.Colors[19] = ImVec4(0.8f, 0.8f, 0.83f, 0.31f);
	s.Colors[20] = ImVec4(0.06f, 0.05f, 0.07f, 1.0f);
	s.Colors[21] = ImVec4(0.250237f, 0.236482f, 0.269058f, 0.3f);
	s.Colors[22] = ImVec4(0.24f, 0.23f, 0.29f, 1.0f);
	s.Colors[23] = ImVec4(0.56f, 0.56f, 0.58f, 1.0f);
	s.Colors[24] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	s.Colors[25] = ImVec4(0.56f, 0.56f, 0.58f, 1.0f);
	s.Colors[26] = ImVec4(0.06f, 0.05f, 0.07f, 1.0f);
	s.Colors[27] = ImVec4(0.43f, 0.43f, 0.5f, 0.5f);
	s.Colors[28] = ImVec4(0.1f, 0.4f, 0.75f, 0.78f);
	s.Colors[29] = ImVec4(0.1f, 0.4f, 0.75f, 1.0f);
	s.Colors[30] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	s.Colors[31] = ImVec4(0.56f, 0.56f, 0.58f, 1.0f);
	s.Colors[32] = ImVec4(0.06f, 0.05f, 0.07f, 1.0f);
	s.Colors[33] = ImVec4(0.595f, 0.085f, 0.085f, 1.0f);
	s.Colors[34] = ImVec4(0.49f, 0.07f, 0.07f, 1.0f);
	s.Colors[35] = ImVec4(0.595f, 0.085f, 0.085f, 1.0f);
	s.Colors[36] = ImVec4(0.42f, 0.06f, 0.06f, 1.0f);
	s.Colors[37] = ImVec4(0.42f, 0.06f, 0.06f, 1.0f);
	s.Colors[38] = ImVec4(0.4f, 0.39f, 0.38f, 0.63f);
	s.Colors[39] = ImVec4(0.25f, 1.0f, 0.0f, 1.0f);
	s.Colors[40] = ImVec4(0.4f, 0.39f, 0.38f, 0.63f);
	s.Colors[41] = ImVec4(0.25f, 1.0f, 0.0f, 1.0f);
	s.Colors[42] = ImVec4(0.19f, 0.19f, 0.2f, 1.0f);
	s.Colors[43] = ImVec4(0.31f, 0.31f, 0.35f, 1.0f);
	s.Colors[44] = ImVec4(0.23f, 0.23f, 0.25f, 1.0f);
	s.Colors[45] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	s.Colors[46] = ImVec4(1.0f, 1.0f, 1.0f, 0.06f);
	s.Colors[47] = ImVec4(0.25f, 1.0f, 0.0f, 0.43f);
	s.Colors[48] = ImVec4(1.0f, 1.0f, 0.0f, 0.9f);
	s.Colors[49] = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);
	s.Colors[50] = ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
	s.Colors[51] = ImVec4(0.8f, 0.8f, 0.8f, 0.2f);
	s.Colors[52] = ImVec4(0.1f, 0.1f, 0.1f, 0.73f);
}

void GUI::loadStyle(const std::filesystem::path& name)
{
	ImGuiStyle s;
	gmoFillDefaultStyle(s);
	float scale = 16.f;
	bool exact = false;

	std::vector<char> buf;
	{
		std::ifstream styleFile(name, std::ios::binary);
		if (styleFile)
			buf.assign((std::istreambuf_iterator<char>(styleFile)), std::istreambuf_iterator<char>());
	}

	// saved by the same ImGui version: raw copy. Anything else (old 1.89 file) -> defaults above.
	if (buf.size() == sizeof(ImGuiStyle) || buf.size() == sizeof(ImGuiStyle) + sizeof(float))
	{
		std::memcpy((void*)&s, buf.data(), sizeof(ImGuiStyle));
		if (buf.size() > sizeof(ImGuiStyle))
			std::memcpy(&scale, buf.data() + sizeof(ImGuiStyle), sizeof(float));
		exact = true;
	}

	loadedStyle = s;
	fontScale = scale;
	geode::log::info("[GMO] loadStyle '{}': file={} bytes, sizeof(ImGuiStyle)={}, exact={}", name.string(), buf.size(), sizeof(ImGuiStyle), exact);
}
// GMO-DEFAULT-STYLE-END

void GUI::setFont(const std::filesystem::path& font)
{
	ImFont* fnt = ImGui::GetIO().Fonts->AddFontFromFileTTF(std::string(font.string()).c_str(), fontScale * uiSizeFactor);
	ImGui::GetIO().FontDefault = fnt;
	/* PORT: reloadFontTexture not in this imgui-cocos version */
}

void GUI::setStyle()
{
	float r = Settings::get<float>("menu/window/color/r", 1.f);
	float g = Settings::get<float>("menu/window/color/g", 0.f);
	float b = Settings::get<float>("menu/window/color/b", 0.f);

	if (Settings::get<bool>("menu/window/rainbow/enabled"))
	{
		ImGui::ColorConvertHSVtoRGB(
			ImGui::GetTime() * Settings::get<float>("menu/window/rainbow/speed", .4f),
			Settings::get<float>("menu/window/rainbow/brightness", .8f),
			Settings::get<float>("menu/window/rainbow/brightness"),
			r, g, b
		);

		GUI::loadedStyle.Colors[ImGuiCol_Border] = { r, g, b, 1 };
		GUI::loadedStyle.Colors[ImGuiCol_TitleBg] = { r, g, b, 1 };
		GUI::loadedStyle.Colors[ImGuiCol_TitleBgCollapsed] = { r, g, b, 1 };
		GUI::loadedStyle.Colors[ImGuiCol_TitleBgActive] = { r, g, b, 1 };

		GUI::loadedStyle.Colors[ImGuiCol_Tab] = { r * .85f, g * .85f, b * .85f, 1 };
		GUI::loadedStyle.Colors[ImGuiCol_TabActive] = { r * .85f, g * .85f, b * .85f, 1 };
		GUI::loadedStyle.Colors[ImGuiCol_TabHovered] = { r * .70f, g * .70f, b * .70f, 1 };
		GUI::loadedStyle.Colors[ImGuiCol_TabUnfocused] = { r * .60f, g * .60f, b * .60f, 1 };
		GUI::loadedStyle.Colors[ImGuiCol_TabUnfocusedActive] = { r * .60f, g * .60f, b * .60f, 1 };
	}
	else
	{
		GUI::loadedStyle.Colors[ImGuiCol_TitleBg] = { r, g, b, 1.f };
		GUI::loadedStyle.Colors[ImGuiCol_TitleBgActive] = { r, g, b, 1.f };
		GUI::loadedStyle.Colors[ImGuiCol_TitleBgCollapsed] = { r, g, b, 1.f };
	}
}

bool GUI::shouldRender()
{
	return isVisible && !shortcutLoop;
}
