#version 130

uniform mat4 g_matWorldViewProj;

in vec2 vertexPos;

void main()
{
    gl_Position = g_matWorldViewProj * vec4(vertexPos, 0, 1);
}