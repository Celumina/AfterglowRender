#version 450

// Optionally declare vertices output variables. 
layout(location = 0) out vec3 vertexColor;

vec2 positions[3] = vec2[3](
    vec2(0.0, -0.5), 
    vec2(0.5, 0.5), 
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[3](
    vec3(1.0, 0.0, 0.0), 
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0) 
);

void main() {
    // gl_Position: build-in vertex shader output (necessary)
    // gl_VertexIndex: build in current vertex index
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    vertexColor = colors[gl_VertexIndex];
}