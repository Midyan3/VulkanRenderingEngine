#include "WindowManager.h"
#include "../Win32Window.h" 
#include <stdexcept>

std::unordered_set<Win32Window*> WindowManager::s_windows; 
std::mutex WindowManager::s_windowsMutex;

void WindowManager::RegisterWindow(Win32Window* window) 
{
	try
	{

		std::lock_guard<std::mutex> lock(s_windowsMutex);
		s_windows.insert(window);

	}
	catch (const std::exception& e)
	{

		Debug::DebugOutput::outputDebug(e.what()); 

	}
	catch (...) 
	{
	
		Debug::DebugOutput::outputDebug("Unexpected Error WindowManger Register Window");

	}
}

void WindowManager::UnregisterWindow(Win32Window* window)
{
	try
	{

		std::lock_guard<std::mutex> lock(s_windowsMutex);
		s_windows.erase(window);

	}
	catch (const std::exception& e)
	{

		Debug::DebugOutput::outputDebug(e.what());

	}
	catch (...)
	{

		Debug::DebugOutput::outputDebug("Unexpected Error WindowManger Unregister Window");

	}
}

size_t WindowManager::GetWindowCount()
{
	try
	{

		std::lock_guard<std::mutex> lock(s_windowsMutex);
		return s_windows.size();

	}
	catch (const std::exception& e)
	{

		Debug::DebugOutput::outputDebug(e.what());
		return 0;

	}
	catch (...)
	{

		Debug::DebugOutput::outputDebug("Unexpected Error WindowManger WindowCount");
		return 0; 

	}
}

void WindowManager::CloseAllWindows()
{
	try
	{

		std::lock_guard<std::mutex> lock(s_windowsMutex);
		for (auto window : s_windows) 
		{
			window->Close(); 
		}

		s_windows.clear(); 

	}
	catch (const std::exception& e)
	{

		Debug::DebugOutput::outputDebug(e.what());

	}
	catch (...)
	{

		Debug::DebugOutput::outputDebug("Unexpected Error WindowManger CloseAllWindows");

	}
}

void WindowManager::PollAllWindowEvents()
{
	try 
	{
		MSG msg; 
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) 
		{

			TranslateMessage(&msg); 
			DispatchMessageW(&msg);
	
		}

	}
	catch (const std::exception& e)
	{

		Debug::DebugOutput::outputDebug(e.what());

	}
	catch (...)
	{

		Debug::DebugOutput::outputDebug("Unexpected Error WindowManger PollAllEvents");

	}
}

std::vector<Win32Window*> WindowManager::GetAllWindows()
{
	try
	{
		
		std::lock_guard<std::mutex> lock(s_windowsMutex);
		return std::vector(s_windows.begin(), s_windows.end()); 

	}
	catch (const std::exception& e)
	{

		Debug::DebugOutput::outputDebug(e.what());
		return {}; 

	}
	catch (...)
	{

		Debug::DebugOutput::outputDebug("Unexpected Error WindowManger PollAllEvents");
		return {}; 

	}
}

Win32Window* WindowManager::FindWindowByTitle(const std::string& title)
{
	try
	{

		std::lock_guard<std::mutex> lock(s_windowsMutex);
		for (auto window : s_windows) 
		{

			if (window->GetTitle() == title)
			{

				return window;

			}

		}

		return nullptr; 

	}
	catch (const std::exception& e)
	{

		Debug::DebugOutput::outputDebug(e.what());
		return nullptr; 

	}
	catch (...)
	{

		Debug::DebugOutput::outputDebug("Unexpected Error WindowManger PollAllEvents");
		return nullptr;

	}
}