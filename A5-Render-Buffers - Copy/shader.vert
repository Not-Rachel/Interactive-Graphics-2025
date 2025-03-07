#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 txc;

uniform mat4 model;
uniform mat4 mvp;
uniform mat4 mv;
uniform mat3 mNorm; //Transform normals

out vec3 normals;
out vec3 posVec;
out vec3 worldPosition;
out vec2 textureCoord;

void main(){
	vec4 positions = vec4(pos,1.0);
    gl_Position = mvp * positions;

	worldPosition = (model * positions).xyz;
	posVec = (mv * positions).xyz; 
	normals = normalize(mNorm * normalize(normal));

	textureCoord = txc;

}
