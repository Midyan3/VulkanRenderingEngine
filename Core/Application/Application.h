#pragma once

#include <chrono>
#include "WindowSpec/WindowSpec.h"

using chronoHighResClock = std::chrono::high_resolution_clock; 

class Application {
public:

	//Constructors and Deconstructors
	Application(); 
	Application(int frameRate, int width, int height);
	~Application(); 

	//Users Will Override These
	virtual void Update(float deltaTime) {};
	virtual void Render() {};

	//Engine Implemented Functions. 
	void Run(); 
	void Quit() { m_running = false;}

	//Set Window Settings
	void setWindowOptions(int frameRate, int width, int height); 
	const windowSpec::WindowOptions& getWindowSettings() const; 

private:

	//Running bool and options for window;
	bool m_running = true; 
	windowSpec::WindowOptions options;

};

Application* CreateApplication(); 