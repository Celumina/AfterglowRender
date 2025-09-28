#version 450

// The input variable does not necessarily have to use the same name, 
// they will be linked together using the indexes specified by the location directives.
layout(location = 0) in vec3 vertexColor;

// Fragment shaders must declare an output variable for each framebuffer manually.
// location = 0 means this is first framebuffer. 
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(vertexColor, 1.0);
}