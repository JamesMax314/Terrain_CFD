#version 450

layout(set = 0, binding = 0) uniform sampler3D uTextureSampler;  // Sampler for the 3D texture

layout(location = 0) in vec2 fragTexCoord;  // Incoming texture coordinates
layout(location = 0) out vec4 fragColor;    // Output color

void main() {
    // Map screen coordinates to 3D texture coordinates
    // For example, assume depth is based on screen position (fragTexCoord.y)
    float zCoord = 0.0f;  // You can compute this based on your own logic

    // Sample the 3D texture at the computed 3D coordinates
    vec3 texCoord = vec3(fragTexCoord.x, fragTexCoord.y, zCoord);  // 3D texture coordinates
    fragColor = texture(uTextureSampler, texCoord);  // Sample the texture at the 3D coordinates
}
