#include "Camera.h"

const Debug::DebugOutput Camera::DebugOut;

Camera::Camera(glm::vec3 position,
    glm::vec3 worldUp,
    float yaw,
    float pitch)
    : m_position(position), m_worldUp(worldUp), m_yaw(yaw), m_pitch(pitch) 
{
    m_front = { 0, 0 , -1 };
    UpdateCameraVectors(); 
}

void Camera::UpdateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);

    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

glm::mat4 Camera::GetViewMatrix() const
{
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(
        glm::radians(m_settings.fov),  
        aspectRatio,                 
        m_settings.nearPlane,
        m_settings.farPlane
    );
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime)
{
    float velocity = m_settings.movementSpeed * deltaTime;

    switch (direction)
    {
    case CameraMovement::Forward:
        m_position += m_front * velocity;
        break;
    case CameraMovement::Backward:
        m_position -= m_front * velocity;
        break;
    case CameraMovement::Left:
        m_position -= m_right * velocity;
        break;
    case CameraMovement::Right:
        m_position += m_right * velocity;  
        break;
    case CameraMovement::Up:
        m_position += m_worldUp * velocity;
        break;
    case CameraMovement::Down:
        m_position -= m_worldUp * velocity;
        break;
    default:
        break;
    }
}

void Camera::ValidateAngles()
{
    if (m_pitch > m_settings.maxPitch)
    {
        m_pitch = m_settings.maxPitch;
    }

    if (m_pitch < m_settings.minPitch)
    {
        m_pitch = m_settings.minPitch;
    }

    while (m_yaw > 360.0f) m_yaw -= 360.0f;
    while (m_yaw < 0.0f) m_yaw += 360.0f;
}

void Camera::ValidateFOV()
{
    if (m_settings.fov < m_settings.minFov)
    {
        m_settings.fov = m_settings.minFov;
    }
    if (m_settings.fov > m_settings.maxFov)
    {
        m_settings.fov = m_settings.maxFov;
    }
}

void Camera::ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch)
{
    xOffset *= m_settings.mouseSensitivity;
    yOffset *= m_settings.mouseSensitivity;

    m_yaw += xOffset;    
    m_pitch += yOffset;  

    if (constrainPitch)
    {
        ValidateAngles();
    }

    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float yOffset)
{
    m_settings.fov -= yOffset;

    ValidateFOV();
}

void Camera::SetRotation(float yaw, float pitch)
{
    m_yaw = yaw;
    m_pitch = pitch;
    ValidateAngles();
    UpdateCameraVectors();
}

void Camera::SetPosition(const glm::vec3& position)
{
    m_position = position;
}

void Camera::SetFOV(float fov)
{
    m_settings.fov = fov;
    ValidateFOV();
}

void Camera::SetSettings(const CameraSettings& settings)
{
    m_settings = settings;
    ValidateFOV();
}

std::string Camera::GetCameraInfo() const
{
    std::string info = "Camera Info:\n";
    info += "  Position: (" + std::to_string(m_position.x) + ", " +
        std::to_string(m_position.y) + ", " +
        std::to_string(m_position.z) + ")\n";
    info += "  Yaw: " + std::to_string(m_yaw) + "°, Pitch: " + std::to_string(m_pitch) + "°\n";
    info += "  FOV: " + std::to_string(m_settings.fov) + "°\n";
    return info;
}
