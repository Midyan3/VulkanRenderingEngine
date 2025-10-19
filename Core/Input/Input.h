#pragma once

#include <array>
#include <cstdint>
#include <string>
#include "../DebugOutput/DubugOutput.h"

// input system
// Error codes: 0x0000E000-0x0000EFFF

enum class MouseButton : int
{
    Left = 0,
    Right = 1,
    Middle = 2,
    Count = 3
};

struct MouseState
{
    float x = 0.0f;
    float y = 0.0f;
    float deltaX = 0.0f;
    float deltaY = 0.0f;
    float scrollDelta = 0.0f;

    bool HasMoved() const { return deltaX != 0.0f || deltaY != 0.0f; }
    bool HasScrolled() const { return scrollDelta != 0.0f; }
};

class Input final
{
public:
    // Singleton
    static Input& Get();

    // Non-copyable, non-movable
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;
    Input(Input&&) = delete;
    Input& operator=(Input&&) = delete;

    // Keyboard queries
    bool IsKeyDown(int vk) const;
    bool IsKeyPressed(int vk) const;
    bool IsKeyReleased(int vk) const;
    bool IsAnyKeyDown() const;

    // Mouse queries
    bool IsMouseButtonDown(MouseButton button) const;
    bool IsMouseButtonPressed(MouseButton button) const;

    const MouseState& GetMouseState() const { return m_mouse; }
    float GetMouseX() const { return m_mouse.x; }
    float GetMouseY() const { return m_mouse.y; }
    float GetMouseDeltaX() const { return m_mouse.deltaX; }
    float GetMouseDeltaY() const { return m_mouse.deltaY; }
    float GetScrollDelta() const { return m_mouse.scrollDelta; }

    // Window callbacks
    void OnKeyPressed(int vk);
    void OnKeyReleased(int vk);
    void OnMouseMove(float x, float y);
    void OnMouseButtonPressed(MouseButton button);
    void OnMouseButtonReleased(MouseButton button);
    void OnMouseScroll(float delta);

    // Per-frame management
    void Update();
    void Reset();

    // Utility
    uint32_t GetPressedKeyCount() const;
    bool IsInitialized() const { return !m_firstMouse; }

private:
    Input() = default;
    ~Input() = default;

    static constexpr int kMaxVK = 256;
    static constexpr int kMaxMouse = 3;

    // State arrays
    std::array<uint8_t, kMaxVK> m_keyDown{};
    std::array<uint8_t, kMaxVK> m_keyPressed{};
    std::array<uint8_t, kMaxVK> m_keyReleased{};

    std::array<uint8_t, kMaxMouse> m_mouseDown{};
    std::array<uint8_t, kMaxMouse> m_mousePressed{};

    MouseState m_mouse{};
    float m_lastX = 0.0f;
    float m_lastY = 0.0f;
    bool m_firstMouse = true;

    // Debug
    static const Debug::DebugOutput DebugOut;

    // Helpers
    static constexpr bool InRange(int vk) { return vk >= 0 && vk < kMaxVK; }
    void ReportWarning(const std::string& msg) const;
};

// VK code constants
namespace VK
{
    constexpr int W = 'W', A = 'A', S = 'S', D = 'D', Q = 'Q', E = 'E';
    constexpr int Key0 = '0', Key1 = '1', Key2 = '2', Key3 = '3', Key4 = '4';
    constexpr int Key5 = '5', Key6 = '6', Key7 = '7', Key8 = '8', Key9 = '9';
    constexpr int Space = 0x20, Shift = 0x10, Ctrl = 0x11, Alt = 0x12;
    constexpr int Escape = 0x1B, Tab = 0x09, Enter = 0x0D;
    constexpr int Left = 0x25, Up = 0x26, Right = 0x27, Down = 0x28;
}