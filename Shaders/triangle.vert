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