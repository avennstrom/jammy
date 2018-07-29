#version 130

uniform sampler2D g_texture;
uniform vec4 g_color = vec4(1, 1, 1, 1);

in vec2 texcoord;

out vec4 color;

void main()
{
    vec4 texColor = texture(g_texture, texcoord);
    color = texColor * g_color;
}