#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aJoints;
layout(location = 4) in vec4 aWeights;

uniform mat4 uMVP;
//uniform mat4 uBones[128];
uniform mat4 uBones[200];
out vec3 vNormal;
out vec2 vUV;

void main() {
    //int j0 = int(aJoints.x);
    int j0 = clamp(int(aJoints.x), 0, 199);
    int j1 = int(aJoints.y);
    int j2 = int(aJoints.z);
    int j3 = int(aJoints.w);

    vec4 skinnedPos =
        aWeights.x * (uBones[j0] * vec4(aPos, 1.0)) +
        aWeights.y * (uBones[j1] * vec4(aPos, 1.0)) +
        aWeights.z * (uBones[j2] * vec4(aPos, 1.0)) +
        aWeights.w * (uBones[j3] * vec4(aPos, 1.0));

//vec4 skinnedPos = vec4(aPos, 1.0);
    vec3 skinnedNormal =
        aWeights.x * (mat3(uBones[j0]) * aNormal) +
        aWeights.y * (mat3(uBones[j1]) * aNormal) +
        aWeights.z * (mat3(uBones[j2]) * aNormal) +
        aWeights.w * (mat3(uBones[j3]) * aNormal);

    gl_Position = uMVP * vec4(skinnedPos.xyz, 1.0);
    vNormal = normalize(skinnedNormal);
    vUV = aUV;
}