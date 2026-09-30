#version 330 core

in vec3 vNormal;
in vec2 vUV;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec4 uColor;
uniform bool uHasTexture;

void main() {
    vec4 base = uColor;
    if (uHasTexture) {
        base *= texture(uTexture, vUV);
    }
    FragColor = base;
}