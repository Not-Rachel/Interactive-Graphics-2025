#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 txc;

uniform mat4 mvp;

out vec2 texCoords;


void main(){
	vec4 positions = vec4(pos,1.0);
    gl_Position = mvp * positions;

    texCoords = txc;

}
