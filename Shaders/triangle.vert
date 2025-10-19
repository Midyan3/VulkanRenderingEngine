/*
#version 450

//Takes constant now to rotate triangle.
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 transform;
} push;

void main() {
    gl_Position = push.transform * vec4(inPosition, 1.0);
    fragColor = inColor;
}
*/

#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 model;  // Renamed from 'transform' to "model"
} push;

layout(binding = 0) uniform CameraUBO {
    mat4 view;  
    mat4 projection;  
} camera;

void main() {
    gl_Position = camera.projection * camera.view * push.model * vec4(inPosition, 1.0);
    
    fragColor = inColor;
}


