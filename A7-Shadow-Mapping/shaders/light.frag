#version 330 core
in vec2 texCoords;

out vec4 fragColor;

uniform sampler2D renderedTexture;


void main(){
    vec4 texColor = texture2D(renderedTexture, texCoords);

    fragColor = texColor;
}
