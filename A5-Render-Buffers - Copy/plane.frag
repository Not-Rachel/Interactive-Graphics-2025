in vec2 texCoords;

out vec4 fragColor;

uniform sampler2D renderedTexture;

void main()
{
    vec4 texColor = texture2D(renderedTexture, texCoords);
    if ((texColor.r + texColor.g + texColor.b) < 0.01){
        fragColor = mix(vec4(0.3, 0.6, 0.9, 1.0), texColor, texColor.r + texColor.g + texColor.b);
    }
    else{
        fragColor = texColor;
    }
}