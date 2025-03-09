in vec2 texCoords;

out vec4 fragColor;

uniform sampler2D renderedTexture;
uniform sampler2DShadow shadow; //Depth map

in vec4 lightViewPosition;


void main()
{
    //vec4 texColor = texture2D(renderedTexture, texCoords);
    vec4 shadowColor = vec4(1,0,0,1) * textureProj(shadow, lightViewPosition); //Depth comparison done by GPU
    
    //if ((texColor.r + texColor.g + texColor.b) < 0.01){
    //    fragColor = mix(vec4(0.3, 0.6, 0.9, 1.0), texColor, texColor.r + texColor.g + texColor.b);
    //}
    //else{
    //    fragColor = texColor; 
    //}
    fragColor = shadowColor;
}