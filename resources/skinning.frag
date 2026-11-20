
#version 330 core

in vec3 vNormal;
in vec2 vUV;
in vec3 vViewPosition;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;
uniform bool uHasTexture;
uniform vec4 uFogColor;

const float FOG_DENSITY = 0.25;

void main()
{
    vec4 base = uColor;

    if (uHasTexture) {
        base *= texture(uTexture, vUV);
    }

    // Эквивалент режима GL_EXP2.
    float distanceToCamera = length(vViewPosition);
    float fogFactor = exp(
        -pow(FOG_DENSITY * distanceToCamera, 2.0)
    );

    fogFactor = clamp(fogFactor, 0.0, 1.0);

    FragColor = mix(uFogColor, base, fogFactor);
}
