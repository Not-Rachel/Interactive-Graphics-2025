
#version 330 core
layout (location = 0) in vec3 env_positions;

out vec3 dir;

uniform mat4 vp;

void main()
{
    dir = (vp * vec4(env_positions,1)).xyz;
    gl_Position = vec4(env_positions, 1.0);
} 