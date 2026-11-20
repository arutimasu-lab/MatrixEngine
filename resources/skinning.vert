
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aJoints;
layout(location = 4) in vec4 aWeights;

uniform mat4 uMVP;
uniform mat4 uModelView;
uniform mat4 uBones[200];

out vec3 vNormal;
out vec2 vUV;
out vec3 vViewPosition;

void main()
{
    int j0 = clamp(int(aJoints.x), 0, 199);
    int j1 = clamp(int(aJoints.y), 0, 199);
    int j2 = clamp(int(aJoints.z), 0, 199);
    int j3 = clamp(int(aJoints.w), 0, 199);

    vec4 localPos =
        aWeights.x * (uBones[j0] * vec4(aPos, 1.0)) +
        aWeights.y * (uBones[j1] * vec4(aPos, 1.0)) +
        aWeights.z * (uBones[j2] * vec4(aPos, 1.0)) +
        aWeights.w * (uBones[j3] * vec4(aPos, 1.0));

    vec3 localNormal =
        aWeights.x * (mat3(uBones[j0]) * aNormal) +
        aWeights.y * (mat3(uBones[j1]) * aNormal) +
        aWeights.z * (mat3(uBones[j2]) * aNormal) +
        aWeights.w * (mat3(uBones[j3]) * aNormal);

    vec4 viewPosition = uModelView * vec4(localPos.xyz, 1.0);

    gl_Position = uMVP * vec4(localPos.xyz, 1.0);

    vNormal = normalize(localNormal);
    vUV = aUV;
    vViewPosition = viewPosition.xyz;
}
