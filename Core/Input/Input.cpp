#include "Input.h"

const Debug::DebugOutput Input::DebugOut;

Input& Input::Get()
{
    static Input s_instance;
    return s_instance;
}

bool Input::IsKeyDown(int vk) const
{
    return InRange(vk) && m_keyDown[vk];
}

bool Input::IsKeyPressed(int vk) const
{
    return InRange(vk) && m_keyPressed[vk];
}

bool Input::IsKeyReleased(int vk) const
{
    return InRange(vk) && m_keyReleased[vk];
}

bool Input::IsAnyKeyDown() const
{
    for (auto state : m_keyDown)
    {
        if (state)
            return true;
    }
    return false;
}

bool Input::IsMouseButtonDown(MouseButton button) const
{
    int index = static_cast<int>(button);
    return index >= 0 && index < kMaxMouse && m_mouseDown[index];
}

bool Input::IsMouseButtonPressed(MouseButton button) const
{
    int index = static_cast<int>(button);
    return index >= 0 && index < kMaxMouse && m_mousePressed[index];
}

void Input::OnKeyPressed(int vk)
{
    if (!InRange(vk))
    {
        ReportWarning("Key pressed out of range: " + std::to_string(vk) + ". 0x0000E010");
        return;
    }

    if (!m_keyDown[vk])
    {
        m_keyPressed[vk] = 1;
    }

    m_keyDown[vk] = 1;
}

void Input::OnKeyReleased(int vk)
{
    if (!InRange(vk))
    {
        ReportWarning("Key released out of range: " + std::to_string(vk) + ". 0x0000E020");
        return;
    }

    if (m_keyDown[vk])
    {
        m_keyReleased[vk] = 1;
    }

    m_keyDown[vk] = 0;
}

void Input::OnMouseMove(float x, float y)
{
    if (m_firstMouse)
    {
        m_lastX = x;
        m_lastY = y;
        m_firstMouse = false;
    }

    m_mouse.x = x;
    m_mouse.y = y;

    // Accumulate delta (handles multiple events per frame)
    m_mouse.deltaX += (x - m_lastX);
    m_mouse.deltaY += (y - m_lastY);

    m_lastX = x;
    m_lastY = y;
}

void Input::OnMouseButtonPressed(MouseButton button)
{
    int index = static_cast<int>(button);
    if (index < 0 || index >= kMaxMouse)
    {
        ReportWarning("Mouse button pressed out of range. 0x0000E030");
        return;
    }

    if (!m_mouseDown[index])
    {
        m_mousePressed[index] = 1;
    }

    m_mouseDown[index] = 1;
}

void Input::OnMouseButtonReleased(MouseButton button)
{
    int index = static_cast<int>(button);
    if (index < 0 || index >= kMaxMouse)
    {
        ReportWarning("Mouse button released out of range. 0x0000E040");
        return;
    }

    m_mouseDown[index] = 0;
}

void Input::OnMouseScroll(float delta)
{
    m_mouse.scrollDelta += delta;
}

void Input::Update()
{
    m_keyPressed.fill(0);
    m_keyReleased.fill(0);
    m_mousePressed.fill(0);

    m_mouse.deltaX = 0.0f;
    m_mouse.deltaY = 0.0f;
    m_mouse.scrollDelta = 0.0f;
}

void Input::Reset()
{
    m_keyDown.fill(0);
    m_keyPressed.fill(0);
    m_keyReleased.fill(0);
    m_mouseDown.fill(0);
    m_mousePressed.fill(0);

    m_mouse = MouseState{};
    m_lastX = 0.0f;
    m_lastY = 0.0f;
    m_firstMouse = true;
}

uint32_t Input::GetPressedKeyCount() const
{
    uint32_t count = 0;
    for (auto state : m_keyDown)
    {
        if (state)
            ++count;
    }
    return count;
}

void Input::ReportWarning(const std::string& message) const
{
    DebugOut.outputDebug("Input Warning: " + message);
}
