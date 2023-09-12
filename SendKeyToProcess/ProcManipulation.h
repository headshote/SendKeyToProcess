#pragma once

#include <windows.h>
#include <psapi.h>
#include <string>
#include <vector>

#include <thread>
#include <mutex>

#include <ctype.h>

#include "threadManager.h"

#define MLC "MLC"
#define MLDC "MLDC"
#define MMV "MMV"

struct ProcessData
{
	std::string procNAme;
	std::string pid;
};

struct WindowElData
{
	HWND eHwnd;
	char eClass[512];
	char eTitle[512];
};

enum InputType{
	Key,
	Mouse
};

struct InputCommand
{
	InputType type;
	std::vector<int> params;
};

std::vector<InputCommand> getKeyCodesFromStr(const std::string& input);

std::vector<std::string> splitString(const std::string& input, const std::string& token);

void sendKeysToWindowWithActivation(HWND hwnd, std::vector<InputCommand>& vKeyCodes);	//by activating, sending the target window to the foreground, and then doing some keyboard event calls
void sendKeysBackground(HWND hwnd, std::vector<InputCommand>& vKeyCodes);	//by sending directly the signal about keyboard press to the foreign process, it can be in the foreground

std::vector<WindowElData> getPidHwnds(DWORD processID);
void sendKeysToProcess(DWORD dwProcessID, const std::string& keystr, int numReps, bool activMode, int timeInterval);
void sendKeysToHwnd(HWND hwnd, const std::string& keystr, int numReps, bool activMode, int timeInterval);

ProcessData printProcessNameAndID(DWORD processID);
std::vector<ProcessData> printProcesses();

void stopThread(std::thread::id tId);
bool checkShouldRun();