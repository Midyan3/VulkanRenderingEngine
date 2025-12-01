#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragWorldNormal;
layout(location = 2) out vec3 fragWorldPos; 
layout(location = 3) out vec2 fragTexCoord;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec3 light;
} push;

layout(binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 projection;
} camera;

void main() {
    vec4 world =  push.model * vec4(inPosition, 1.0);
    fragWorldPos = world.xyz;
    gl_Position = camera.projection * camera.view * world;
    fragWorldNormal = mat3(push.model) * inNormal;
    fragColor = inColor;
    fragTexCoord = inTexCoord; 
}