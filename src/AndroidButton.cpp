// Android: small round draggable button. Tap = open/close the GDMegaOverlay UI.
// Drag = move the button (position is saved). The button survives scene changes.
#include <Geode/Geode.hpp>
#if __has_include(<Geode/ui/SceneManager.hpp>)
#include <Geode/ui/SceneManager.hpp>
#elif __has_include(<Geode/utils/SceneManager.hpp>)
#include <Geode/utils/SceneManager.hpp>
#endif

#include "AndroidButton.hpp"
#include "GUI/GUI.h"

using namespace geode::prelude;

#ifdef GEODE_IS_ANDROID

namespace
{
	constexpr float kRadius = 17.f;       // visual radius (GD units)
	constexpr float kHitPadding = 8.f;    // easier to hit with a thumb
	constexpr float kDragThreshold = 6.f; // below this it's a tap, not a drag

	class GMOFloatingButton : public CCNode, public CCTouchDelegate
	{
		CCDrawNode* m_circle = nullptr;
		CCPoint m_touchStart = {0, 0};
		CCPoint m_nodeStart = {0, 0};
		bool m_tracking = false;
		bool m_dragged = false;
		bool m_registered = false;

		void redraw(bool pressed)
		{
			m_circle->clear();
			m_circle->drawDot({0, 0}, kRadius, ccc4f(1.f, 1.f, 1.f, pressed ? 0.55f : 0.30f));
			m_circle->drawDot({0, 0}, kRadius - 2.f, ccc4f(0.10f, 0.10f, 0.16f, pressed ? 0.95f : 0.80f));
		}

		CCPoint clamp(CCPoint p)
		{
			auto win = CCDirector::get()->getWinSize();
			p.x = std::clamp(p.x, kRadius, win.width - kRadius);
			p.y = std::clamp(p.y, kRadius, win.height - kRadius);
			return p;
		}

		bool hits(CCTouch* touch)
		{
			auto loc = touch->getLocation(); // already in GL coordinates
			return ccpDistance(loc, this->getPosition()) <= kRadius + kHitPadding;
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

			m_circle = CCDrawNode::create();
			this->addChild(m_circle);
			redraw(false);

			auto label = CCLabelBMFont::create("M", "bigFont.fnt");
			label->setScale(0.45f);
			label->setOpacity(230);
			this->addChild(label);

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
				// very high priority so the game/menus don't eat the touch, but we only
				// swallow touches that start on the button itself
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
			redraw(true);
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
			redraw(false);

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
			redraw(false);
		}
	};

	GMOFloatingButton* s_button = nullptr;
}

#include <Geode/modify/CCDirector.hpp>

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
