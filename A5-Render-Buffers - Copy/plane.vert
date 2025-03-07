#version 330 core
layout(location = 0) in vec3 plane_pos;
layout(location = 1) in vec2 plane_tex_coords;

out vec2 texCoords;

uniform mat4 mvp;

void main()
{
    vec4 pos = vec4(plane_pos, 1.0);
    gl_Position = mvp * pos;
    texCoords = plane_tex_coords;
}