#version 450

layout(set = 0, binding = 0) uniform MeshUniform {
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 invTransModel;
} meshUniform;

layout(set = 3, binding = 0) uniform Balabala {
    float TEST;
    float TEST1;
    float TEST2;
    float TEST3;
} balabala;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec2 inTexCoord0;

layout(location = 0) out vec4 outWorldPostion;
layout(location = 1) out vec3 outWorldNormal;
layout(location = 2) out vec4 outVertColor;
layout(location = 3) out vec2 outTexCoord0;


void main() {
    gl_Position.x = balabala.TEST;
    gl_Position = meshUniform.projection * meshUniform.view * meshUniform.model * vec4(inPosition, 1.0);
    outWorldPostion = gl_Position;
    outWorldNormal = (meshUniform.invTransModel * vec4(inNormal, 0.0)).xyz;
    outVertColor = inColor;
    outTexCoord0 = inTexCoord0 * vec2(1.0, -1.0);
}