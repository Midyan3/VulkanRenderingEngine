#include "WindowSpec.h"

windowSpec::WindowOptions::WindowOptions(int targetFrameRate, int width, int height)
    : targetFrameRate(targetFrameRate), width(width), height(height) {}

windowSpec::WindowOptions::WindowOptions(int targetFrameRate, int width, int height, std::string title)
    : targetFrameRate(targetFrameRate), width(width), height(height), title(title) {}

void windowSpec::WindowOptions::setSettings(int targetFrameRate, int width, int height) {
    this->targetFrameRate = targetFrameRate;
    this->width = width;
    this->height = height;
}