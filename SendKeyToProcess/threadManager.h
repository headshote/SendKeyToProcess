#pragma once

#include <vector>

#include <memory>
#include <thread>
#include <mutex>

namespace threadManager
{
	void addThread(std::shared_ptr<std::thread> thrd);
	void detachThreads();
	void joinThreads();
	void sendStopToAllThreads(void(*stopThread)(std::thread::id));
	void onThreadCompleted(std::thread::id tId);
	void checkCompletedIds();
}