#pragma once

#include <mutex>
#include <functional>
#include "messages/MapFileOperation.h"
#include "ui/mainframe/ScreenUpdateBlocker.h"

namespace ui
{

class MapFileProgressHandler
{
private:
	std::size_t _msgSubscription;

	std::unique_ptr<ScreenUpdateBlocker> _blocker;

	std::mutex _lock;
	bool _wasCancelled;

	std::size_t _level;

public:
	MapFileProgressHandler();

	~MapFileProgressHandler();

private:
	void handleFileOperation(map::FileOperation& msg);
	void dispatchWithLockAndCatch(const std::function<void()>& function);
};

}
