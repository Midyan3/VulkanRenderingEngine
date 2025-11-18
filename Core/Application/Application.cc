#include "Application.h"
#include <thread>
#include <iostream>

// Update(float deltaTime) and Render() not defined here and are virtual;

//Constructor And The Deconstructor
Application::Application() : options(60, 1920, 1080) {}

Application::Application(int frameRate, int width, int height)
	: options(frameRate, width, height) {}

Application::~Application() {}

// Engine Specific Functions

void Application::Run() {
	//Intailizing the lastTime to start run as well as target time for frames
	auto lastTime = chronoHighResClock::now();
	const float targetFrameTime = 1.0f / options.targetFrameRate; 
	// Calling update every frame and calculate deltaTime;  
	while (m_running) {
		auto timeNow = chronoHighResClock::now();
		auto deltaTime = std::chrono::duration<float>(timeNow - lastTime).count(); 
		Update(deltaTime); 
		Render();
		const float frameTime = std::chrono::duration<float>(chronoHighResClock::now() - timeNow).count();
		{
			using namespace std; 
			std::cout << "FPS: " << 1.0f / deltaTime << '\n'; 
		}
		if (options.capped)
		{
			if (frameTime < targetFrameTime) {
				std::this_thread::sleep_for(std::chrono::duration<float>(targetFrameTime - frameTime)); 
			}
		}
		lastTime = timeNow; 
	}
}

// Window Specific Options

void Application::setWindowOptions(int frameRate, int width, int height) { options.setSettings(frameRate, width, height); }

const windowSpec::WindowOptions& Application::getWindowSettings() const { return options;}

