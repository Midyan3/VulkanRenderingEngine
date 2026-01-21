#pragma once

#include "../../Headers/GlmConfig.h"
#include <memory>
#include "../DebugOutput/DubugOutput.h"

#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>


// Camera movement directions
enum class CameraMovement
{
    Forward,
    Backward,
    Left,
    Right,
    Up,
    Down
};

// Camera configuration
struct CameraSettings
{
    float movementSpeed = 20.0f;     
    float mouseSensitivity = 0.1f;   
    float fov = 45.0f;              
    float nearPlane = 0.1f;          
    float farPlane = 1000.0f;        

    float minPitch = -89.0f;    
    float maxPitch = 89.0f;
    float minFov = 1.0f;
    float maxFov = 180.0f;
};

class Camera
{
public:
    // Constructors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = -90.0f,
        float pitch = 0.0f);

    ~Camera() = default;

    // Non-copyable, movable
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera(Camera&&) noexcept = default;
    Camera& operator=(Camera&&) noexcept = default;

    // Update camera
    //void Update(float deltaTime);

    // Manual movement control (for direct input)
    void ProcessKeyboard(CameraMovement direction, float deltaTime);
    void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yOffset);

    // Get transformation matrices
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;

    // Getters for camera state
    glm::vec3 GetPosition() const { return m_position; }
    glm::vec3 GetFront() const { return m_front; }
    glm::vec3 GetUp() const { return m_up; }
    glm::vec3 GetRight() const { return m_right; }
    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }
    float GetFOV() const { return m_settings.fov; }

    // Setters
    void SetPosition(const glm::vec3& position);
    void SetRotation(float yaw, float pitch);
    void SetFOV(float fov);

    // Settings access
    CameraSettings& GetSettings() { return m_settings; }
    const CameraSettings& GetSettings() const { return m_settings; }
    void SetSettings(const CameraSettings& settings);

    // Debug info
    std::string GetCameraInfo() const;

private:
    // Camera coordinate system
    glm::vec3 m_position;
    glm::vec3 m_front;  
    glm::vec3 m_up;        
    glm::vec3 m_right;      
    glm::vec3 m_worldUp;    

    float m_yaw;            // Rotation around Y-axis (left/right)
    float m_pitch;          // Rotation around X-axis (up/down)

    // Camera settings
    CameraSettings m_settings;

    // Debug output
    static const Debug::DebugOutput DebugOut;

    // Internal helpers
    void UpdateCameraVectors();
    void ValidateAngles();
    void ValidateFOV();

    void ReportWarning(const std::string& message) const
    {
        DebugOut.outputDebug("Camera Warning: " + message);
    }
};