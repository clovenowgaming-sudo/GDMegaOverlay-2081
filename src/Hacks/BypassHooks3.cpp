#include "../Settings.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/CCTextInputNode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

using namespace geode::prelude;

// Text Length / Character Filter: lifted right before each insert
class $modify(GMOBypassTextInput, CCTextInputNode)
{
	bool onTextFieldInsertText(CCTextFieldTTF* sender, char const* text, int len, enumKeyCodes key)
	{
		if (Settings::get<bool>("hack/text_length", false))
			m_maxLabelLength = 9999;

		if (Settings::get<bool>("hack/char_filter", false))
		{
			m_allowedChars = "";
			m_filterSwearWords = false;
		}

		return CCTextInputNode::onTextFieldInsertText(sender, text, len, key);
	}
};

// Object Limit Bypass: paste / duplicate never hits the object limit
class $modify(GMOBypassEditor, LevelEditorLayer)
{
	CCArray* createObjectsFromString(gd::string const& str, bool noUndo, bool noLimit)
	{
		if (Settings::get<bool>("hack/object_limit", false))
			noLimit = true;
		return LevelEditorLayer::createObjectsFromString(str, noUndo, noLimit);
	}
};

// Copy Hack: the clone button is always shown
class $modify(GMOBypassInfo, LevelInfoLayer)
{
	bool init(GJGameLevel* level, bool challenge)
	{
		if (!LevelInfoLayer::init(level, challenge))
			return false;

		if (Settings::get<bool>("hack/copy_hack", false) && m_cloneBtn)
		{
			m_cloneBtn->setVisible(true);
			m_cloneBtn->setEnabled(true);
		}
		return true;
	}
};
