// Android: small round draggable button. Tap = open/close the GDMegaOverlay UI.
// Drag = move the button (position is saved).
#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>

#include "AndroidButton.hpp"
#include "GUI/GUI.h"

using namespace geode::prelude;

#ifdef GEODE_IS_ANDROID

namespace
{
	constexpr float kHitPadding = 8.f;
	constexpr float kDragThreshold = 6.f;

	class GMOFloatingButton : public CCNode, public CCTouchDelegate
	{
		CCSprite* m_sprite = nullptr;
		float m_radius = 17.f;
		CCPoint m_touchStart = {0, 0};
		CCPoint m_nodeStart = {0, 0};
		bool m_tracking = false;
		bool m_dragged = false;
		bool m_registered = false;

		void setPressed(bool pressed)
		{
			if (!m_sprite) return;
			m_sprite->setColor(pressed ? ccColor3B{150, 150, 150} : ccColor3B{255, 255, 255});
			m_sprite->setOpacity(pressed ? 255 : 200);
		}

		CCPoint clamp(CCPoint p)
		{
			auto win = CCDirector::get()->getWinSize();
			p.x = std::clamp(p.x, m_radius, win.width - m_radius);
			p.y = std::clamp(p.y, m_radius, win.height - m_radius);
			return p;
		}

		bool hits(CCTouch* touch)
		{
			return ccpDistance(touch->getLocation(), this->getPosition()) <= m_radius + kHitPadding;
		}

	public:
		static GMOFloatingButton* create()
		{
			auto ret = new GMOFloatingButton();
			if (ret && ret->init())
			{
				ret->autorelease();
				return ret;
			}
			CC_SAFE_DELETE(ret);
			return nullptr;
		}

		bool init() override
		{
			if (!CCNode::init())
				return false;

			// round button sprite from the game's own sheet
			m_sprite = CCSprite::createWithSpriteFrameName("GJ_button_01.png");
			if (m_sprite)
			{
				m_sprite->setScale(0.6f);
				m_radius = m_sprite->getContentSize().width * m_sprite->getScale() * 0.5f;
				this->addChild(m_sprite);
			}

			auto label = CCLabelBMFont::create("M", "bigFont.fnt");
			label->setScale(0.5f);
			this->addChild(label);

			setPressed(false);

			auto win = CCDirector::get()->getWinSize();
			float x = Mod::get()->getSavedValue<float>("android_button/x", win.width - 30.f);
			float y = Mod::get()->getSavedValue<float>("android_button/y", win.height * 0.6f);
			this->setPosition(clamp({x, y}));
			this->setZOrder(100000);
			return true;
		}

		void onEnter() override
		{
			CCNode::onEnter();
			if (!m_registered)
			{
				CCTouchDispatcher::get()->addTargetedDelegate(this, -9999, true);
				m_registered = true;
			}
		}

		void onExit() override
		{
			if (m_registered)
			{
				CCTouchDispatcher::get()->removeDelegate(this);
				m_registered = false;
			}
			m_tracking = false;
			CCNode::onExit();
		}

		bool ccTouchBegan(CCTouch* touch, CCEvent*) override
		{
			if (!this->isVisible() || !hits(touch))
				return false;

			m_tracking = true;
			m_dragged = false;
			m_touchStart = touch->getLocation();
			m_nodeStart = this->getPosition();
			setPressed(true);
			return true;
		}

		void ccTouchMoved(CCTouch* touch, CCEvent*) override
		{
			if (!m_tracking)
				return;

			auto delta = touch->getLocation() - m_touchStart;
			if (!m_dragged && ccpLength(delta) < kDragThreshold)
				return;

			m_dragged = true;
			this->setPosition(clamp(m_nodeStart + delta));
		}

		void ccTouchEnded(CCTouch*, CCEvent*) override
		{
			if (!m_tracking)
				return;

			m_tracking = false;
			setPressed(false);

			if (m_dragged)
			{
				Mod::get()->setSavedValue<float>("android_button/x", this->getPositionX());
				Mod::get()->setSavedValue<float>("android_button/y", this->getPositionY());
			}
			else
				GUI::toggle();
		}

		void ccTouchCancelled(CCTouch*, CCEvent*) override
		{
			m_tracking = false;
			setPressed(false);
		}
	};

	GMOFloatingButton* s_button = nullptr;
}

// Keeps the button attached to whatever scene is currently running.
static void gmoAttachButton()
{
	auto scene = CCDirector::get()->getRunningScene();
	if (!scene || typeinfo_cast<CCTransitionScene*>(scene))
		return;

	if (!s_button)
	{
		s_button = GMOFloatingButton::create();
		if (!s_button)
			return;
		s_button->retain();
	}

	if (s_button->getParent() == scene)
		return;

	s_button->removeFromParentAndCleanup(false);
	scene->addChild(s_button, 100000);
}

class $modify(GMOButtonDirector, CCDirector)
{
	void drawScene()
	{
		gmoAttachButton();
		CCDirector::drawScene();
	}
};

void AndroidButton::ensureCreated() {}

#else

void AndroidButton::ensureCreated() {}

#endif
