#pragma once 

#include <string>
#include "../../../../DebugOutput/DubugOutput.h"
#include <unordered_set>
#include <vector>
#include <mutex>

class Win32Window;

class WindowManager {
private:

	static std::unordered_set<Win32Window*> s_windows; 
	static std::mutex s_windowsMutex; 

public:

	static void RegisterWindow(Win32Window* window); 
	static void UnregisterWindow(Win32Window* window); 

	static size_t GetWindowCount(); 
	static void CloseAllWindows(); 
	static void PollAllWindowEvents(); 

	static std::vector<Win32Window*> GetAllWindows(); 
	static Win32Window* FindWindowByTitle(const std::string& title); 

};