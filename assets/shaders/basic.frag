#version 330 core

out vec4 fragColor;

in vec3 vertexColor;
in vec2 vertexUV;
in vec3 vertexWorldPosition;
in vec3 vertexWorldNormal;
uniform vec3 uTint;
uniform sampler2D uTexture;
uniform sampler2D uNormalMap;
uniform int uNormalMapping;
uniform int uUseTexture;
uniform int uMaterialType;
uniform vec3 uEyePosition;
uniform int uPointLight;
uniform vec3 uLightPosition;
uniform vec3 uLightDirection;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform float uWaterIor;
uniform vec3 uWaterReflectionColor;
uniform float uRoughness;
uniform float uMetallic;

const int SAND = 1;
const int WATER = 2;
const int UNLIT = 3;
const int MASONRY = 4;
const int METAL = 5;
const float PI = 3.14159265359;

vec3 surfaceNormal() {
    vec3 n = normalize(vertexWorldNormal);
    if (uNormalMapping == 0 || (uMaterialType != SAND && uMaterialType != MASONRY))
        return n;

    // Axes of increasing U and V in the terrain mesh. Horizontal UVs use X/Z;
    // upright faces use X or Z across and -Y down. This also preserves the
    // handedness of mirrored projections on opposite sides of a block.
    vec3 tangent = abs(n.x) > 0.5 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 bitangent = abs(n.y) > 0.5 ? vec3(0, 0, 1) : vec3(0, -1, 0);
    vec3 sampleNormal = texture(uNormalMap, vertexUV).rgb * 2.0 - 1.0;
    return normalize(mat3(tangent, bitangent, n) * sampleNormal);
}

vec3 fresnelSchlick(float cosTheta, vec3 f0) {
    return f0 + (1.0 - f0) * pow(1.0 - clamp(cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 f0, float roughness) {
    vec3 grazingReflectance = max(vec3(1.0 - roughness), f0);
    return f0 + (grazingReflectance - f0)
        * pow(1.0 - clamp(cosTheta, 0.0, 1.0), 5.0);
}

float distributionGGX(vec3 normal, vec3 halfway, float roughness) {
    float alpha = roughness * roughness;
    float alphaSquared = alpha * alpha;
    float nDotH = max(dot(normal, halfway), 0.0);
    float denominator = nDotH * nDotH * (alphaSquared - 1.0) + 1.0;
    return alphaSquared / max(PI * denominator * denominator, 1e-6);
}

float geometrySchlickGGX(float nDotDirection, float roughness) {
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return nDotDirection / max(nDotDirection * (1.0 - k) + k, 1e-6);
}

float geometrySmith(vec3 normal, vec3 view, vec3 light, float roughness) {
    float nDotV = max(dot(normal, view), 0.0);
    float nDotL = max(dot(normal, light), 0.0);
    return geometrySchlickGGX(nDotV, roughness)
        * geometrySchlickGGX(nDotL, roughness);
}

vec3 cookTorranceGGX(vec3 normal, vec3 view, vec3 light, vec3 albedo,
                     float metallic, float roughness, vec3 f0) {
    float nDotV = max(dot(normal, view), 0.0);
    float nDotL = max(dot(normal, light), 0.0);
    if (nDotV <= 0.0 || nDotL <= 0.0)
        return vec3(0.0);
    vec3 halfway = normalize(view + light);
    float vDotH = max(dot(view, halfway), 0.0);
    vec3 fresnel = fresnelSchlick(vDotH, f0);
    float distribution = distributionGGX(normal, halfway, roughness);
    float geometry = geometrySmith(normal, view, light, roughness);
    vec3 specular = distribution * geometry * fresnel
        / max(4.0 * nDotV * nDotL, 1e-5);
    vec3 diffuseWeight = (vec3(1.0) - fresnel) * (1.0 - metallic);
    vec3 diffuse = diffuseWeight * albedo / PI;
    return diffuse + specular;
}

vec3 linearToSrgb(vec3 color) {
    color = max(color, vec3(0.0));
    bvec3 lowRange = lessThanEqual(color, vec3(0.0031308));
    vec3 low = color * 12.92;
    vec3 high = 1.055 * pow(color, vec3(1.0 / 2.4)) - 0.055;
    return mix(high, low, lowRange);
}

void main() {
    vec4 baseColor = vec4(vertexColor * uTint, 1.0);
    if (uUseTexture != 0)
        baseColor *= texture(uTexture, vertexUV);
    if (uMaterialType == UNLIT) {
        fragColor = baseColor;
        return;
    }

    vec3 light = normalize(uLightDirection);
    vec3 radiance = uLightColor;
    if (uPointLight != 0) {
        vec3 offset = uLightPosition - vertexWorldPosition;
        float d = length(offset);
        light = offset / max(d, 0.001);
        // Smoothly blend the moving pool of light into cool ambient fill.
        radiance *= (1.0 - smoothstep(6.0, 11.0, d)) / (1.0 + 0.06*d + 0.045*d*d);
    }

    vec3 n = surfaceNormal();
    vec3 view = normalize(uEyePosition - vertexWorldPosition);
    float nDotL = max(dot(n, light), 0.0);

    if (uMaterialType == SAND || uMaterialType == WATER) {
        float nDotV = max(dot(n, view), 0.0);
        float metallic = clamp(uMetallic, 0.0, 1.0);
        float roughness = clamp(uRoughness, 0.045, 1.0);
        vec3 f0 = vec3(0.04);
        if (uMaterialType == WATER) {
            float ior = max(uWaterIor, 1.0);
            float ratio = (ior - 1.0) / (ior + 1.0);
            f0 = vec3(ratio * ratio);
        }
        f0 = mix(f0, baseColor.rgb, metallic);

        vec3 directBrdf = cookTorranceGGX(
            n, view, light, baseColor.rgb, metallic, roughness, f0);
        vec3 diffuseWeight = (vec3(1.0)
            - fresnelSchlickRoughness(nDotV, f0, roughness))
            * (1.0 - metallic);
        vec3 ambient = diffuseWeight * baseColor.rgb * uAmbientColor;
        vec3 lighting = ambient + directBrdf * radiance * nDotL;
        if (uMaterialType == WATER) {
            vec3 environmentFresnel =
                fresnelSchlickRoughness(nDotV, f0, roughness);
            lighting += uWaterReflectionColor * environmentFresnel * 1.2;
        }
        fragColor = vec4(linearToSrgb(lighting), baseColor.a);
        return;
    }

    float rDotV = max(dot(reflect(-light, n), view), 0.0);
    float diffuse = 0.82;
    float specular = 0.12;
    float shininess = 20.0;
    if (uMaterialType == MASONRY) {
        specular = 0.23; shininess = 32.0;
    } else if (uMaterialType == METAL) {
        specular = 0.78; shininess = 72.0;
    }
    float highlight = nDotL > 0.0 ? pow(rDotV, shininess) : 0.0;
    vec3 color = baseColor.rgb * (uAmbientColor + radiance * nDotL * diffuse)
               + radiance * specular * highlight;
    fragColor = vec4(color, baseColor.a);
}
