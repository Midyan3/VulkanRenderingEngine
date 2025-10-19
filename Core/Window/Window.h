
#pragma once

#include <string>
#include <memory>
#include <functional>
#include "../../Core/Input/Input.h"
#include "../Application/WindowSpec/WindowSpec.h"

//The abstract base Window class to be used for the implementation of window creation across different platform

class Input;

class Window {
private:

	struct KeyCreation; 

public: 

	//Exposing Key creator to derived classes
	using Key = KeyCreation;

	// Callback apporach will be done instead of having it in pollEvent loop
	using CloseRequestedCallback = std::function<void()>;
	using ResizedCallback = std::function<void(int width, int height)>;
	using KeyCallback = std::function<void(int keyCode, bool isPressed)>;
	using MouseButtonCallback = std::function<void(int button, bool isPressed, int x, int y)>;
	using MouseMoveCallback = std::function<void(int x, int y)>;
	using MouseWheelCallback = std::function<void(float steps)>;

	//	Deconstrutor
	virtual ~Window() = default; 

	// Set of Window function for class to imnplement from base. TODO: Add the win, macos, and, linux support
	virtual void Show() = 0; 
	virtual void Hide() = 0; 
	virtual void Close() = 0; 
	virtual bool ShouldClose() = 0; 

	//The Window properties functions
	virtual void SetTitle(const std::string& title) = 0;
	virtual void SetSize(int width, int height) = 0;
	virtual int GetWidth() const = 0; 
	virtual int GetHeight() const = 0; 
	virtual bool IsVisible() const = 0;

	// Event handling function to handle window events
	virtual void PollEvents() = 0; 

	// Event registration for callback
	virtual void OnCloseRequested(CloseRequestedCallback callback) = 0;
	virtual void OnResized(ResizedCallback callback) = 0;
	virtual void OnKeyEvent(KeyCallback callback) = 0;
	virtual void OnMouseButton(MouseButtonCallback callback) = 0;
	virtual void OnMouseMove(MouseMoveCallback callback) = 0;
	virtual void OnMouseScroll(MouseWheelCallback callback) = 0;

	virtual bool SetUpMouseAndKeyboard() = 0;

	static std::unique_ptr<Window> Create(const windowSpec::WindowOptions& options);

private:

	struct KeyCreation {
		
		friend std::unique_ptr<Window> Window::Create(const windowSpec::WindowOptions& options);
	
	private:

		KeyCreation() = default;
	
	};

};