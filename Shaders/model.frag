#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldNormal;
layout(location = 2) in vec3 fragWorldPos; 

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec3 light;
} push;

void main() {
    vec3 light = push.light - fragWorldPos;
    float brightness = dot(normalize(fragWorldNormal), normalize(light)); 
    brightness = max(0.0, brightness) + 0.15;
    
    vec3 litColor = fragColor * brightness;

    outColor = vec4(litColor, 1.0);
}