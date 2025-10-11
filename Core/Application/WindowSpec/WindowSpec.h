#pragma once 

#include <string>

namespace windowSpec {
	struct WindowOptions {
		WindowOptions(int targetFrameRate, int width, int height);
		WindowOptions(int targetFrameRate, int width, int height, std::string title);
		void setSettings(int targetFrameRate, int width, int height);
		int targetFrameRate;
		int width;
		int height;
		std::string title = "GameEngine";
	};
}
