#version 330 core

// The rasterizer stores this output color in the framebuffer.
out vec4 fragColor;

in vec3 vertexColor;
in vec2 vertexUV;
uniform vec3 uTint;
uniform sampler2D uTexture;
uniform int uUseTexture;

void main() {
    vec4 baseColor = vec4(vertexColor * uTint, 1.0);
    if (uUseTexture != 0) {
        baseColor *= texture(uTexture, vertexUV);
    }
    fragColor = baseColor;
}
