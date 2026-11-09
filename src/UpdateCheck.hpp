#pragma once

namespace UpdateCheck
{
	// Checks the latest GitHub release once per session. force = user pressed "Check for updates".
	void run(bool force = false);
}
