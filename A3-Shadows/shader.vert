#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 normal;


uniform mat4 mvp;
uniform mat4 mv;
uniform mat3 mNorm; //Transform normals

out vec3 normals;
out vec3 posVec;
out vec3 worldPosition;

void main(){
	vec4 positions = vec4(pos,1);
    gl_Position = mvp * positions;

	vec3 P = (vec3(mv * vec4(pos,1))); 
	vec3 N = (mNorm * normalize(normal));

	posVec = P; //Camera
	normals = normalize(N);
	worldPosition = normalize(pos);
}
