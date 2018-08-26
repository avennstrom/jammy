#version 130

uniform mat4 g_matWorldViewProj;

in vec2 vertexPos;
in vec2 vertexTexcoord;

out vec2 texcoord;

void main()
{
    gl_Position = g_matWorldViewProj * vec4(vertexPos, 0, 1);
    texcoord = vertexTexcoord;
}