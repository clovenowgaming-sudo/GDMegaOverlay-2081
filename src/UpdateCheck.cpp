#include "UpdateCheck.hpp"

#include <Geode/Geode.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/web.hpp>

#include <nlohmann/json.hpp>

#include <cstdio>
#include <string>

using namespace geode::prelude;

namespace
{
	// owner/repo that publishes the GitHub Releases
	constexpr const char* kRepo = "clovenowgaming-sudo/GDMegaOverlay-2081";
	bool s_checked = false;

	void parseVersion(std::string const& s, int out[3])
	{
		out[0] = out[1] = out[2] = 0;
		const char* p = s.c_str();
		if (*p == 'v' || *p == 'V')
			p++;
		std::sscanf(p, "%d.%d.%d", &out[0], &out[1], &out[2]);
	}

	bool isNewer(std::string const& latest, std::string const& current)
	{
		int a[3], b[3];
		parseVersion(latest, a);
		parseVersion(current, b);
		for (int i = 0; i < 3; i++)
		{
			if (a[i] != b[i])
				return a[i] > b[i];
		}
		return false;
	}
}

void UpdateCheck::run(bool force)
{
	if (s_checked && !force)
		return;
	s_checked = true;

	std::string url = fmt::format("https://api.github.com/repos/{}/releases/latest", kRepo);

	async::spawn(
		web::WebRequest().userAgent("GDMegaOverlay-Updater").get(url),
		[force](web::WebResponse resp)
		{
			if (!resp.ok())
			{
				if (force)
					FLAlertLayer::create("Update check", "Could not reach GitHub.", "OK")->show();
				return;
			}

			auto body = resp.string();
			if (body.isErr())
				return;

			auto j = nlohmann::json::parse(body.unwrap(), nullptr, false);
			if (j.is_discarded() || !j.is_object() || !j.contains("tag_name"))
			{
				if (force)
					FLAlertLayer::create("Update check", "No release found yet.", "OK")->show();
				return;
			}

			std::string tag = j["tag_name"].get<std::string>();
			std::string current = Mod::get()->getVersion().toVString();

			if (!isNewer(tag, current))
			{
				if (force)
					FLAlertLayer::create("Update check", "You are up to date.", "OK")->show();
				return;
			}

			if (!force && Mod::get()->getSavedValue<std::string>("update/notified", "") == tag)
				return;
			Mod::get()->setSavedValue<std::string>("update/notified", tag);

			std::string page = j.value("html_url", fmt::format("https://github.com/{}/releases", kRepo));
			std::string notes = j.value("body", std::string());
			if (notes.size() > 300)
				notes = notes.substr(0, 300) + "...";

			createQuickPopup(
				"GD Mega Overlay update",
				fmt::format("Version <cg>{}</c> is available (you have {}).\n\n{}", tag, current, notes),
				"Later", "Open page",
				[page](FLAlertLayer*, bool open)
				{
					if (open)
						web::openLinkInBrowser(page);
				});
		});
}
