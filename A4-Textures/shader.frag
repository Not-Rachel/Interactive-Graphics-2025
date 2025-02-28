#version 330 core

//Lighting:
uniform vec3 lightPos;
uniform float alpha;
uniform vec3 ambient;
uniform sampler2D tex_kd;
uniform sampler2D tex_ks;


in vec3 normals;
in vec3 posVec;
in vec3 worldPosition;
in vec2 textureCoord;

out vec4 fragColor;
void main(){
    vec3 lightDir = normalize(lightPos - worldPosition);		
    vec3 viewDir = normalize(-posVec);
    vec3 halfVec = normalize(lightDir + viewDir); //Angle between lightDir and viewDir

    float diffuse = max(dot(normalize(normals), lightDir),0.01);
    float spec = 0.0;

    if ( diffuse > 0.0){
        spec = pow(max(dot(normalize(normals), halfVec),0.0),alpha);
    }
    vec3 Kd = vec3(1.0,1.0,1.0);
    vec3 Ks = vec3(0.0,0.0,1.0);


    float trans = 1.0;

    //if (textureCoord){

    //Diffuse Texture
    vec4 clr_kd = texture2D(tex_kd, textureCoord); //Return RGBA value
    trans = clr_kd[3];
    Kd = vec3(clr_kd[0],clr_kd[1],clr_kd[2]); 

    //Specular Texture
    vec4 clr_ks = texture2D(tex_ks, textureCoord); //Return RGBA value
    trans = clr_ks[3];
    Ks = vec3(clr_ks[0],clr_ks[1],clr_ks[2]) ; 
	//}
    
    vec4 color = vec4((Kd * diffuse) + (Ks*spec),trans) + vec4(ambient, 0.4);
    
    fragColor = color;
}
