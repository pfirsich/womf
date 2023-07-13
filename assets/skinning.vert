#version 330 core

const int MAX_JOINTS = 64;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;
uniform mat4 jointMatrices[MAX_JOINTS];

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
// layout(location = 2) in vec3 attrTangent;
layout(location = 3) in vec2 attrTexCoords0;
// layout(location = 4) in vec2 attrTexCoords1;
// layout(location = 5) in vec2 attrColor;
layout(location = 6) in vec4 attrJoints; // Does not work with ivec4?
layout(location = 7) in vec4 attrJointWeights;

out vec2 texCoords;
out vec3 normal; // view space

void main()
{
    mat4 skinMatrix = attrJointWeights.x * jointMatrices[int(attrJoints.x)]
        + attrJointWeights.y * jointMatrices[int(attrJoints.y)]
        + attrJointWeights.z * jointMatrices[int(attrJoints.z)]
        + attrJointWeights.w * jointMatrices[int(attrJoints.w)];
    texCoords = attrTexCoords0;
    normal = normalMatrix * attrNormal;
    gl_Position
        = projectionMatrix * viewMatrix * modelMatrix * skinMatrix * vec4(attrPosition, 1.0);
}
