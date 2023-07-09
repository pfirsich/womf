#version 330 core

uniform sampler2D texture;
uniform vec4 color;

in vec2 texCoords;

out vec4 fragColor;

void main()
{
    fragColor = color * texture2D(texture, texCoords);
}
