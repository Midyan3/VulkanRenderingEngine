#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldNormal;
layout(location = 2) in vec3 fragWorldPos; 
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec3 light;
} push;

void main() {
    vec3 light = push.light - fragWorldPos;
    float brightness = dot(normalize(fragWorldNormal), normalize(light)); 
    brightness = max(0.0, brightness) + 0.15;
    
    vec3 texColor = texture(texSampler, fragTexCoord).rgb;
    
    vec3 litColor = texColor * brightness;

    outColor = vec4(litColor, 1.0);
}