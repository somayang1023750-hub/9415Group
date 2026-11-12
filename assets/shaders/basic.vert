#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in mat4 aInstanceModel;
layout(location = 7) in vec3 aWorldNormal;
layout(location = 8) in vec4 aParticleColor;

uniform mat4 uMvp;
uniform mat4 uModel;
uniform mat4 uViewProjection;
uniform int uInstanced;
uniform int uParticlePass;
uniform int uMaterialType;

out vec3 vertexColor;
out float particleAlpha;
out vec2 vertexUV;
out vec3 vertexWorldPosition;
out vec3 vertexWorldNormal;

void main() {
    vertexColor = uParticlePass != 0 ? aParticleColor.rgb : aColor;
    particleAlpha = uParticlePass != 0 ? aParticleColor.a : 1.0;
    vertexUV = aTexCoord;
    vec4 vertexPosition = vec4(aPosition, 1.0);
    mat4 model = uModel;
    if (uInstanced != 0) {
        model = aInstanceModel;
    }
    vec4 worldPosition = model * vertexPosition;
    vertexWorldPosition = worldPosition.xyz;
    if (uMaterialType == 3) {
        vertexWorldNormal = vec3(0.0, 1.0, 0.0);
    } else {
        mat3 normalMatrix = transpose(inverse(mat3(model)));
        vertexWorldNormal = normalize(normalMatrix * aWorldNormal);
    }
    if (uInstanced != 0) {
        gl_Position = uViewProjection * worldPosition;
    } else {
        gl_Position = uMvp * vertexPosition;
    }
}
