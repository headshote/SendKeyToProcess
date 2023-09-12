#include "threadManager.h"

using namespace threadManager;

static std::vector<std::shared_ptr<std::thread>> runningThreads;
static std::vector<std::thread::id> completedIds;
static std::mutex tlMutex;

void threadManager::addThread(std::shared_ptr<std::thread> thrd)
{
	runningThreads.push_back(thrd);
}

/*
detaches the threads in the list and removes them
*/
void threadManager::detachThreads()
{
	for (auto iter = runningThreads.begin(); iter != runningThreads.end();)
	{
		std::shared_ptr<std::thread> th = *iter;
		if (th->joinable())
		{
			th->detach();

			runningThreads.erase(iter);
			iter = runningThreads.begin();
		}
		else
			++iter;
	}
}

/*
Joins the threads in the list and removes them
*/
void threadManager::joinThreads()
{
	for (auto iter = runningThreads.begin(); iter != runningThreads.end();)
	{
		std::shared_ptr<std::thread> th = *iter;
		if (th->joinable())
		{
			th->join();

			runningThreads.erase(iter);
			iter = runningThreads.begin();
		}
		else
			++iter;
	}
}

void threadManager::sendStopToAllThreads(void(*stopThread)(std::thread::id))
{
	for (int i = 0; i < runningThreads.size(); ++i)
	{
		stopThread(runningThreads[i]->get_id());
	}
}

void threadManager::onThreadCompleted(std::thread::id tId)
{
	std::unique_lock<std::mutex>(tlMutex);
	completedIds.push_back(tId);
}

void threadManager::checkCompletedIds()
{	
	std::unique_lock<std::mutex>(tlMutex);
	for (int i = 0; i < completedIds.size(); ++i)
	{
		for (auto iter = runningThreads.begin(); iter != runningThreads.end();)
		{
			std::shared_ptr<std::thread> thrd = *iter;
			std::thread::id tId = thrd->get_id();

			if (tId == completedIds[i])
			{
				if (thrd->joinable())
					thrd->join();

				runningThreads.erase(iter);
				iter = runningThreads.begin();
				break;
			}
			else
			{
				++iter;
			}
		}
	}

	if (completedIds.size() > 0)
		completedIds.clear();
	
}