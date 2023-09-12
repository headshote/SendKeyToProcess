#include "ProcManipulation.h"

//Program state vars
static bool eventsWithActivation;
static int numCalls;

///	Multithreading hax
static std::vector<std::thread::id> killThreads;
static std::mutex m1;

void stopThread(std::thread::id tId)
{
	std::unique_lock<std::mutex> lck1(m1);
	killThreads.push_back(tId);
}

bool checkShouldRun()
{
	std::unique_lock<std::mutex> lck1(m1);
	for (auto iter = killThreads.begin(); iter != killThreads.end(); ++iter)
		if (*iter == std::this_thread::get_id())
		{
			killThreads.erase(iter);
			return false;
		}
	return true;
}
///

//////
std::vector<InputCommand> getKeyCodesFromStr(const std::string& input)
{
	std::vector<InputCommand> commands;
	for (int i = 0; i < input.length(); ++i)
	{
		char ch = input.at(i);
		if (ch == '{')
		{
			InputCommand ic;
			ic.type = Mouse;

			int closingTag = input.find_first_of('}', i);
			std::string mCommand = input.substr(i + 1, closingTag - i-1);
			
			std::vector<std::string> subCommands = splitString(mCommand, ",");

			for (int j = 0; j < subCommands.size(); ++j)
			{
				std::string& subComm = subCommands[j];
				if (subComm == MLC)
				{
					ic.params.push_back(WM_LBUTTONDOWN);
					ic.params.push_back(MOUSEEVENTF_LEFTDOWN);
				}
				else if (subComm == MLDC)
				{
					ic.params.push_back(WM_LBUTTONDBLCLK);
					ic.params.push_back(MOUSEEVENTF_LEFTDOWN);
				}
				else if (subComm == MMV)
				{
					ic.params.push_back(WM_MOUSEMOVE);
					ic.params.push_back(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE);
				}
				else
				{
					ic.params.push_back(atoi(subComm.c_str()));
				}
			}

			commands.push_back(ic);

			i = closingTag;	//skip insides of a tag while searchin for the next command, loop iteration will add 1 to this
		}
		else
		{
			InputCommand ic;
			ic.type = Key;
			ic.params.push_back(toupper(ch));

			commands.push_back(ic);
		}
	}
	return commands;
}

std::vector<std::string> splitString(const std::string& input, const std::string& token)
{
	std::vector<std::string> retval;

	int tokenPos = -1;
	int oldTokenPos;

	do
	{
		oldTokenPos = tokenPos+1;

		++tokenPos;
		tokenPos = input.find_first_of(token, tokenPos);

		retval.push_back(input.substr(oldTokenPos, tokenPos - oldTokenPos));
	}while (tokenPos != -1);

	return retval;
}
//////


/////
void sendKeysToWindowWithActivation(HWND hwnd, std::vector<InputCommand>& vKeyCodes)
{
	//Make the window of the target pid currently active
	SetForegroundWindow(hwnd);
	SetActiveWindow(hwnd);
	SetFocus(hwnd);
	//detach inputs from the currently active window to the target window
	AttachThreadInput(GetWindowThreadProcessId(GetForegroundWindow(), NULL), GetCurrentThreadId(), TRUE);

	for (int i = 0; i < vKeyCodes.size(); ++i)
	{
		InputCommand& ic = vKeyCodes[i];
		if (ic.type == Key)
		{
			INPUT ip;
			// Set up a generic keyboard event.
			ip.type = INPUT_KEYBOARD;
			ip.ki.wScan = 0; // hardware scan code for key
			ip.ki.time = 0;
			ip.ki.dwExtraInfo = 0;

			// Press the key
			ip.ki.wVk = ic.params[0]; // virtual-key code for the "a" key
			ip.ki.dwFlags = 0; // 0 for key press
			UINT nPressEvents = SendInput(1, &ip, sizeof(INPUT));

			// Release the "A" key
			ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
			UINT nReleaseEvents = SendInput(1, &ip, sizeof(INPUT));
		}
		else if(ic.type == Mouse)
		{
			int mEvent = ic.params[1];
			POINT pt = { ic.params[ic.params.size() - 2], ic.params[ic.params.size()-1] };

			ClientToScreen(hwnd, &pt);

			//Send the input to the window as if it was currenta
			mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, pt.x*(65536 / GetSystemMetrics(SM_CXSCREEN)), pt.y*(65536 / GetSystemMetrics(SM_CYSCREEN)), 0, 0);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
		}
	}
}

void sendKeysBackground(HWND hwnd, std::vector<InputCommand>& vKeyCodes)
{
	for (int i = 0; i < vKeyCodes.size(); ++i)
	{
		InputCommand& ic = vKeyCodes[i];
		if (ic.type == Key)
		{
			PostMessage(hwnd, WM_KEYUP, ic.params[0], 1);
		}
		else if (ic.type == Mouse)
		{
			int mEvent = ic.params[0];
			POINT pt = { ic.params[ic.params.size() - 2], ic.params[ic.params.size() - 1] };

			//ClientToScreen(hwnd, &pt);
			//PostMessage(hwnd, WM_LBUTTONDBLCLK, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));
			PostMessage(hwnd, mEvent, MK_LBUTTON, MAKELPARAM(pt.x, pt.y));
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			PostMessage(hwnd, WM_LBUTTONUP, 0, MAKELPARAM(pt.x, pt.y));
		}
	}
	//PostMessage(hwnd, WM_QUIT, 0, 0); // -- just for the reference, closes the window
}

std::vector<WindowElData> getPidHwnds(DWORD processID)
{
	std::vector<WindowElData> pidHwndData;

	HWND hwnd = NULL;
	do	//loop over windows/subwindows of the process (main ones)
	{
		hwnd = FindWindowEx(NULL, hwnd, NULL, NULL);

		HWND chldHwnd = NULL;
		do	//loop over subelements (subwindows) of the current "top-level" windows in a process
		{
			chldHwnd = FindWindowEx(hwnd, chldHwnd, NULL, NULL);

			DWORD chldPID = 0;
			DWORD chldThread = GetWindowThreadProcessId(chldHwnd, &chldPID);

			if (chldPID == processID && chldThread != NULL)
			{
				WindowElData wData;
				wData.eHwnd = chldHwnd;
				GetWindowText(chldHwnd, wData.eTitle, sizeof(wData.eTitle));
				GetClassName(chldHwnd, wData.eClass, sizeof(wData.eClass));
				pidHwndData.push_back(wData);
			}
		} while (chldHwnd != NULL);

	} while (hwnd != NULL);

	return pidHwndData;
}

void sendKeysToProcess(DWORD dwProcessID, const std::string& keystr, int numReps, bool activMode, int timeInterval)
{
	std::vector<WindowElData> pidHwndData = getPidHwnds(dwProcessID);

	std::vector<InputCommand> keyCodes = getKeyCodesFromStr(keystr);

	int j = 0;
	while (j < numReps)
	{
		for (int i = 0; i < pidHwndData.size(); ++i)
		{
			HWND chldHwnd = pidHwndData[i].eHwnd;

			if (!activMode)		//TODO: some state var here
				sendKeysBackground(chldHwnd, keyCodes);
			else
				sendKeysToWindowWithActivation(chldHwnd, keyCodes);
		}

		if (timeInterval > 0)
			std::this_thread::sleep_for(std::chrono::milliseconds(timeInterval));

		if (!checkShouldRun())
		{
			threadManager::onThreadCompleted(std::this_thread::get_id());
			return;
		}

		++j;
	}

	threadManager::onThreadCompleted(std::this_thread::get_id());
}

void sendKeysToHwnd(HWND hwnd, const std::string& keystr, int numReps, bool activMode, int timeInterval)
{
	std::vector<InputCommand> keyCodes = getKeyCodesFromStr(keystr);

	int j = 0;
	while (j < numReps)
	{		
		if (!activMode)		//TODO: some state var here
			sendKeysBackground(hwnd, keyCodes);
		else
			sendKeysToWindowWithActivation(hwnd, keyCodes);

		if (timeInterval > 0)
			std::this_thread::sleep_for(std::chrono::milliseconds(timeInterval));

		if (!checkShouldRun())
		{
			threadManager::onThreadCompleted(std::this_thread::get_id());
			return;
		}

		++j;
	}

	threadManager::onThreadCompleted(std::this_thread::get_id());
}
/////


/////
ProcessData printProcessNameAndID(DWORD processID)
{
	// ! GetLastError ! - useful for error checking on winapi function calls (when they return flase or zero, etc.)
	TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

	// Get a handle to the process.
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_ALL_ACCESS, FALSE, processID);

	// Get the process name.
	if (NULL != hProcess)
	{
		HMODULE hMod;
		DWORD cbNeeded;

		if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
		{
			GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
		}
	}

	// Release the handle to the process.
	CloseHandle(hProcess);

	return ProcessData{ std::string(szProcessName),
		std::to_string(processID) };
}

std::vector<ProcessData> printProcesses()
{
	std::vector<ProcessData> processEntries;

	// Get the list of process identifiers.
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
	{
		return processEntries;
	}

	// Calculate how many process identifiers were returned.
	cProcesses = cbNeeded / sizeof(DWORD);

	// Print the name and process identifier for each process.
	for (i = 0; i < cProcesses; i++)
	{
		if (aProcesses[i] != 0)
		{
			processEntries.push_back(printProcessNameAndID(aProcesses[i]));
		}
	}

	return processEntries;
}
//////