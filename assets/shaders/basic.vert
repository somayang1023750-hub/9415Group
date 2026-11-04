#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uMvp;

out vec3 vertexColor;
out vec2 vertexUV;

void main() {
    vertexColor = aColor;
    vertexUV = aTexCoord;
    gl_Position = uMvp * vec4(aPosition, 1.0);
}
