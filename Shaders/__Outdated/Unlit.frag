#version 450

layout(set = 0, binding = 1) uniform SceneUniform {
	vec4 dirLightDirectionIntensity;
	vec4 dirLightColor;
	vec4 cameraPosition;
	vec4 cameraVector;
	vec2 screenResolution;
	float time;
} sceneUniform;

layout(set = 2, binding = 1) uniform sampler2D albedo;

layout(location = 0) in vec4 worldPostion;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec4 vertColor;
layout(location = 3) in vec2 texCoord0;

layout(location = 0) out vec4 fragColor;

void main() {
    vec4 color = texture(albedo, texCoord0);
    if (color.w < 0.5) {
        // discard;
    }
    color *= 
        max(dot(sceneUniform.dirLightDirectionIntensity.xyz, worldNormal), 0.0)
        * sceneUniform.dirLightDirectionIntensity.w;
    fragColor = color;
    // fragColor = vec4(texCoord0.x, texCoord0.y, 0.0, 1.0);
    // fragColor = vec4(worldNormal, 1.0);
}