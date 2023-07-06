#version 330 core

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;

layout(location = 0) in vec3 attrPosition;
layout(location = 1) in vec3 attrNormal;
layout(location = 3) in vec2 attrTexCoords;

out vec2 texCoords;
out vec3 normal; // view space

void main()
{
    texCoords = attrTexCoords;
    normal = normalMatrix * attrNormal;
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(attrPosition, 1.0);
}
