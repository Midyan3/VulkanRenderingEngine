#pragma once 

#include "../../Window.h"
#include "../WidnowSetting/WindowSetting.h"
#include "../../../DebugOutput/DubugOutput.h"
#include "WindowManager/WindowManager.h"
#include "WindowStateEnum/WindowStateEnum.h"
#include <Windows.h>
#include <unordered_map>
#include <vector>        
#include <functional>  
#include <optional>
#include <string>        

//Win32 Implementation of Window interface

class Win32Window : public Window {

private: 

	//Vectors of callbakcs
	std::vector<CloseRequestedCallback> m_closeCallbacks;
	std::vector<ResizedCallback> m_resizedCallbacks;
	std::vector<KeyCallback> m_keyCallbacks;
	std::vector<MouseButtonCallback> m_mouseButtonCallbacks;
	std::vector<MouseMoveCallback> m_mouseMoveCallbacks;
	std::vector<MouseWheelCallback> m_mouseScroll;

	// Member variable
	const static Debug::DebugOutput DebugOut;
	HWND m_hwnd = nullptr; 
	int m_width, m_height; 
	std::string m_title; 
	static const wchar_t* s_windowClassName;
	static bool s_classRegistered;
	WindowStateENUM::VisibleState m_visible = WindowStateENUM::VisibleState::NotVisible;      
	WindowStateENUM::CloseState m_shouldClose = WindowStateENUM::CloseState::Open;

	/*
	* Old Appoarch, better appoarch was used which avoids race coniditons thus this was discarded.
	//Static Windows Map
	static std::unordered_map<HWND, Win32Window*> s_windowMap; 
	*/

	static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// Instance method that can access callbacks
	LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

	bool RegisterWindowClass();   
	bool CreateWindowInstance();  
	void CleanupWindow();         

	void OnWin32Close();                                          
	void OnWin32Resize(int width, int height);                   
	void OnWin32KeyEvent(WPARAM wParam, bool isPressed);         
	void OnWin32MouseButton(WPARAM wParam, LPARAM lParam, std::pair<WindowStateENUM::MouseButtonState, bool>, std::pair<int, int>);
	void OnWin32MouseMove(LPARAM lParam);
	void OnWin32MouseScroll(uint32_t delta);

	//Helper
	std::optional<std::pair<WindowStateENUM::MouseButtonState, bool>> GetMouseButtonInfo(UINT msg);

public:

	Win32Window(Key key, int width, int height, const std::string& title); 
	~Win32Window() override;
	Win32Window(const Win32Window&) = delete;
	Win32Window& operator=(const Win32Window&) = delete;
	//Win32Window(Win32Window&& other) noexcept;
	//Win32Window& operator=(Win32Window&& other) noexcept;
	
	// Set of Window function for class to imnplement from base. TODO: Add the win, macos, and, linux support
	virtual void Show() override; 
	virtual void Hide() override;
	virtual void Close() override;
	virtual bool ShouldClose() override;

	//The Window properties functions
	virtual void SetTitle(const std::string& title) override;
	virtual void SetSize(int width, int height) override;
	virtual int GetWidth() const override { return m_width; }
	virtual int GetHeight() const override { return m_height; }
	virtual bool IsVisible() const override;
	virtual bool SetUpMouseAndKeyboard() override;

	std::string GetTitle() const { return m_title;} 

	// Event handling function to handle window events
	virtual void PollEvents() override;

	// Event registration for callback
	virtual void OnCloseRequested(CloseRequestedCallback callback) override;
	virtual void OnResized(ResizedCallback callback) override;
	virtual void OnKeyEvent(KeyCallback callback) override;
	virtual void OnMouseButton(MouseButtonCallback callback) override;
	virtual void OnMouseMove(MouseMoveCallback callback) override;
	virtual void OnMouseScroll(MouseWheelCallback callback) override;


	bool IsInitialized() const { return m_hwnd == nullptr; }

	#ifdef _WIN32
		HWND GetHWND() const { return m_hwnd; }
		HINSTANCE GetHINSTANCE() const { return GetModuleHandleW(nullptr); }
	#endif

};