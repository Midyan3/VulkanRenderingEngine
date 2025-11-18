#include "Win32Window.h"
#include <windowsx.h>
#include <stdexcept>

bool Win32Window::s_classRegistered = false;
const Debug::DebugOutput Win32Window::DebugOut; 
const wchar_t* Win32Window::s_windowClassName = L"GameEngine";
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Win32Window::Win32Window(Key key, int width, int height, const std::string& title) 
	: m_width(width), m_height(height), m_title(title) 
{
			
	if (!RegisterWindowClass())
		throw std::runtime_error("Failed to register the window class");

	if (!CreateWindowInstance())
		throw std::runtime_error("Failed to create a window"); 

	WindowManager::RegisterWindow(this); 

}

void Win32Window::CleanupWindow() 
{
	if (m_hwnd) {

		// Unregister from WindowManager
		WindowManager::UnregisterWindow(this);

		// Destroy the window if it still exists
		if (IsWindow(m_hwnd)) {
			DestroyWindow(m_hwnd);
		}

		m_hwnd = nullptr;
	}
}

Win32Window::~Win32Window() 
{
	CleanupWindow();
}

bool Win32Window::RegisterWindowClass() 
{
	if (s_classRegistered) {
		return s_classRegistered;
	}
	
	WNDCLASSW wc = {}; 
	wc.lpfnWndProc = StaticWindowProc; 
	wc.hInstance = GetModuleHandleW(nullptr); 
	wc.lpszClassName = s_windowClassName; 
	ATOM result = RegisterClassW(&wc); 
	s_classRegistered = (result != 0); 
	
	return s_classRegistered; 

}

bool Win32Window::CreateWindowInstance()
{

	//Creates a winodw forr application
	m_hwnd = CreateWindowExW(
		0,
		s_windowClassName,
		std::wstring(m_title.begin(), m_title.end()).c_str(),
		WS_OVERLAPPEDWINDOW,
		0,
		0,
		m_width,
		m_height,
		nullptr,
		nullptr,
		GetModuleHandle(nullptr),
		this
	); 

	if (!m_hwnd)
		return false; 

	/*
	* Old Apporach. Now is handled when window is created stored in each window
	//Add window to the static map of other windows
	s_windowMap[m_hwnd] = this; 	
	*/

	return true; 

}

void Win32Window::PollEvents() 
{
	MSG msg;
	while (PeekMessageW(&msg, m_hwnd, 0, 0, PM_REMOVE)) 
	{
		
		TranslateMessage(&msg); 
		DispatchMessageW(&msg); 

	}
}

std::optional<std::pair<WindowStateENUM::MouseButtonState, bool>> Win32Window::GetMouseButtonInfo(UINT msg)
{
	static const std::unordered_map<UINT, std::pair<WindowStateENUM::MouseButtonState, bool>> mouseMap 
    = {
		{WM_LBUTTONDOWN, {WindowStateENUM::MouseButtonState::Left, true}},
		{WM_LBUTTONUP,   {WindowStateENUM::MouseButtonState::Left, false}},
		{WM_RBUTTONDOWN, {WindowStateENUM::MouseButtonState::Right, true}},
		{WM_RBUTTONUP,   {WindowStateENUM::MouseButtonState::Right, false}},
		{WM_MBUTTONDOWN, {WindowStateENUM::MouseButtonState::Middle, true}},
		{WM_MBUTTONUP,   {WindowStateENUM::MouseButtonState::Middle, false}}
	};

	auto it = mouseMap.find(msg);
	
	if (it != mouseMap.end()) 
	{
	
		return it->second;
	
	}

	return std::nullopt;
}

void Win32Window::OnWin32Close()
{
	for (auto& callback : m_closeCallbacks) {
		try
		{

			callback();

		}
		catch (const std::exception& e)
		{

			DebugOut.outputDebug(e.what());

		}
		catch (...)
		{

			DebugOut.outputDebug("Unexpected Error OnWin32Close");

		}
	}
	
	m_shouldClose = WindowStateENUM::CloseState::Close; 
}

void Win32Window::OnWin32Resize(int width, int height)
{
	m_width = width; 
	m_height = height; 

	for (auto& callback : m_resizedCallbacks) {
		try 
		{

			callback(width, height); 

		}
		catch (const std::exception& e) 
		{

			DebugOut.outputDebug(e.what()); 
		
		}
		catch (...)
		{

			DebugOut.outputDebug("Unexpected Error OnWin32Resize"); 

		}
	}
}

void Win32Window::OnWin32KeyEvent(WPARAM wParam, bool isPressed) 
{
	int keyCode = static_cast<int>(wParam); 

	for (auto& callback : m_keyCallbacks) {
		try 
		{

			callback(keyCode, isPressed);

		}
		catch (const std::exception& e) 
		{

			DebugOut.outputDebug(e.what());

		}
		catch (...) 
		{

			DebugOut.outputDebug("Unexpected Error OnWin32KeyEvent");

		}
	}
}

void Win32Window::OnWin32MouseMove(LPARAM lParam) 
{
	const int XCord = GET_X_LPARAM(lParam), YCord = GET_Y_LPARAM(lParam); 

	//std::cout << "X: " << XCord << " Y: " << YCord << std::endl;

	for (auto& callback : m_mouseMoveCallbacks) {
		try 
		{
		
			callback(XCord, YCord);
		
		}
		catch (const std::exception& e) 
		{
		
			DebugOut.outputDebug(e.what());
		
		}
		catch (...) 
		{
			
			DebugOut.outputDebug("Unexpected Error OnWin32MouseMove");
		
		}
	}
}

void Win32Window::OnWin32MouseButton(WPARAM wParam, LPARAM lParam, std::pair<WindowStateENUM::MouseButtonState, bool> MouseState, std::pair<int, int> Cord) 
{
	for (auto& callback : m_mouseButtonCallbacks) {
		try
		{

			callback(MouseState.first, MouseState.second, Cord.first, Cord.second);

		}
		catch (const std::exception& e)
		{

			DebugOut.outputDebug(e.what());

		}
		catch (...)
		{

			DebugOut.outputDebug("Unexpected Error OnWin32MouseButton");

		}
	}

}

void Win32Window::OnWin32MouseScroll(uint32_t delta)
{

	for (auto& callback : m_mouseScroll)
	{
		try
		{
			callback(delta);
		}
		catch (const std::exception& e)
		{
			DebugOut.outputDebug(e.what());
		}
		catch (...)
		{
			DebugOut.outputDebug("Unexpected Error OnMouseScroll");
		}
	}

}

LRESULT Win32Window::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) 
{
	try 
	{

		auto mouseInfor = GetMouseButtonInfo(msg); 
		if (mouseInfor.has_value()) 
		{
			
			const int XCord = LOWORD(lParam), YCord = HIWORD(lParam);
			OnWin32MouseButton(wParam , lParam, std::make_pair(mouseInfor->first, mouseInfor->second), std::make_pair(XCord, YCord));
		
		}

		switch (msg) 
		{
			case WM_NCDESTROY:
			{

				SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, 0); 
				return 0; 

			}
				break; 
			case WM_CLOSE:
				
				OnWin32Close(); 
				
				break; 
			case WM_SIZE:
			{

				int width = LOWORD(lParam); 
				int height = HIWORD(lParam); 
				OnWin32Resize(width, height); 

			}
				break; 
			case WM_KEYDOWN:
			{

				OnWin32KeyEvent(wParam, true); 

			}
				break; 
			case WM_KEYUP:
			{

				OnWin32KeyEvent(wParam, false);

			}
				break;
			case WM_MOUSEWHEEL: 
			{

				const float steps = static_cast<short>(HIWORD(wParam)) / 120.0f;
				OnWin32MouseScroll(steps);

			}
				break;
			case WM_MOUSEMOVE:
			{

				OnWin32MouseMove(lParam); 

			}
				break;
			default: 

				//Return default if message handling not iimplemented yet
				return DefWindowProcW(m_hwnd, msg, wParam, lParam); 
		}

		return 0; 

	}
	catch (const std::exception& e) 
	{

		DebugOut.outputDebug(e.what()); 

	}
	catch (...)
	{

		DebugOut.outputDebug("Unexpected Error HandleMessage");
		return DefWindowProcW(m_hwnd, msg, wParam, lParam);

	}
	
	return 1; 

}

LRESULT CALLBACK Win32Window::StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	try 
	{
		if (ImGui::GetCurrentContext())
		{
			if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
				return true; 
		}

		if (msg == WM_NCCREATE) 
		{

			auto storedData = reinterpret_cast<CREATESTRUCTW*>(lParam); 
			auto self = static_cast<Win32Window*>(storedData->lpCreateParams); 
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self)); 
			self->m_hwnd = hwnd;

		}
		
		//Retrieving self to call handle message
		auto self = reinterpret_cast<Win32Window*>
			(GetWindowLongPtrW(hwnd, GWLP_USERDATA)); 

		if (self) 
		{
			try 
			{

				return self->HandleMessage(msg, wParam, lParam); 

			}
			catch (const std::exception& e)
			{

				DebugOut.outputDebug(e.what()); 

			}
			catch (...) 
			{

				DebugOut.outputDebug("Unexpected Error Window MSG Handler"); 

			}
		}

		return DefWindowProcW(hwnd, msg, wParam, lParam); 

	}
	catch (const std::exception& e)
	{
	
		DebugOut.outputDebug(e.what()); 
		
	}
	catch (...)		
	{

		DebugOut.outputDebug("Unexpected Error"); 
		
	}

	return DefWindowProcW(hwnd, msg, wParam, lParam); 

}

void Win32Window::Show() 
{
	ShowWindow(m_hwnd, SW_SHOW);
	m_visible = WindowStateENUM::VisibleState::Visible;
}

void Win32Window::Hide() 
{
	ShowWindow(m_hwnd, SW_HIDE);
	m_visible = WindowStateENUM::VisibleState::NotVisible;
}

bool Win32Window::ShouldClose() 
{
	return m_shouldClose == WindowStateENUM::CloseState::Close;
}

bool Win32Window::IsVisible() const 
{
	return m_visible == WindowStateENUM::VisibleState::Visible;
}

void Win32Window::SetTitle(const std::string& title) {
	try {
		m_title = title;
		std::wstring wideTitle(title.begin(), title.end());  

		if (!SetWindowTextW(m_hwnd, wideTitle.c_str())) 
		{  
		
			DWORD error = GetLastError();
			DebugOut.outputDebug("SetWindowTextW failed with error: " + std::to_string(error));
		
		}
	
	}
	catch (const std::exception& e) 
	{
	
		DebugOut.outputDebug(e.what());  
	
	}
}

void Win32Window::Close() {
	if (m_hwnd) 
	{
		if (!DestroyWindow(m_hwnd)) 
		{
		
			DWORD error = GetLastError();
			DebugOut.outputDebug("DestroyWindow failed: " + std::to_string(error));
		
		}
		
		m_hwnd = nullptr;
	}
}

bool Win32Window::SetUpMouseAndKeyboard()
{

	OnKeyEvent([](int keyCode, bool isPressed)
		{
			if (isPressed)
			{
				Input::Get().OnKeyPressed(keyCode);
			}
			else
			{
				Input::Get().OnKeyReleased(keyCode);
			}
		});

	OnMouseMove([](int x, int y)
		{
			Input::Get().OnMouseMove(static_cast<float>(x), static_cast<float>(y));
		});

	OnMouseButton([](int button, bool isPressed, int x, int y)
		{
			MouseButton btn;
			switch (button)
			{
			case 0: btn = MouseButton::Left; break;
			case 1: btn = MouseButton::Right; break;
			case 2: btn = MouseButton::Middle; break;
			default: return;  
			}

			if (isPressed)
			{
				Input::Get().OnMouseButtonPressed(btn);
			}
			else
			{
				Input::Get().OnMouseButtonReleased(btn);
			}
		});

	OnMouseScroll([](float delta)
		{
			Input::Get().OnMouseScroll(delta);
		});

	return true;

}

void Win32Window::SetSize(int width, int height) 
{
	m_width = width;
	m_height = height;
	SetWindowPos(m_hwnd, nullptr, 0, 0, width, height,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

//Callback functions; 
void Win32Window::OnCloseRequested(CloseRequestedCallback callback) { m_closeCallbacks.push_back(callback); }
void Win32Window::OnResized(ResizedCallback callback) { m_resizedCallbacks.push_back(callback); }
void Win32Window::OnKeyEvent(KeyCallback callback) { m_keyCallbacks.push_back(callback); }
void Win32Window::OnMouseButton(MouseButtonCallback callback) { m_mouseButtonCallbacks.push_back(callback); }
void Win32Window::OnMouseMove(MouseMoveCallback callback) { m_mouseMoveCallbacks.push_back(callback); }
void Win32Window::OnMouseScroll(MouseWheelCallback callback) { m_mouseScroll.push_back(callback);  }