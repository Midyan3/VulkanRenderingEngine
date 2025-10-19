#include "Window.h"
#include "OS-Windows/Win32/Win32Window.h"
#include "../../Core/Input/Input.h"
#include <memory>

std::unique_ptr<Window> Window::Create(const windowSpec::WindowOptions& options)
{
#ifdef _WIN32
	return std::make_unique<Win32Window>(
		KeyCreation{},
		options.width,
		options.height,
		options.title
	);
#else
	return nullptr;
#endif
}