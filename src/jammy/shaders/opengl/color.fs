#version 130

uniform vec4 g_color = vec4(1, 1, 1, 1);

out vec4 color;

void main()
{
    color = g_color;
}