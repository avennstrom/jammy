#version 130

//uniform mat4 g_matWorldViewProj;

in vec2 vertexPos;
in vec2 vertexTexcoord;

out vec2 texcoord;

void main()
{
    gl_Position.xy = vertexPos;
    gl_Position.z = 0.0;
    gl_Position.w = 1.0;
    texcoord = vertexTexcoord;
}