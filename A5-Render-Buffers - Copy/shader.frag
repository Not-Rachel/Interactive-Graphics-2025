#version 330 core

//Lighting:
uniform vec3 lightPos;
uniform vec3 camera;
uniform float alpha;
uniform vec3 ambient;
uniform sampler2D tex_kd;
uniform sampler2D tex_ks;
uniform samplerCube env_kd;

in vec3 normals;
in vec3 posVec;
in vec3 worldPosition;
in vec2 textureCoord;

out vec4 fragColor;
void main(){
    vec3 lightDir = normalize(lightPos - worldPosition);		
    vec3 viewDir = normalize(camera - worldPosition);
    vec3 halfVec = normalize(lightDir + viewDir); //Angle between lightDir and viewDir

    vec3 reflected = normalize(reflect(-viewDir, normals));
    vec3 correctedReflected = vec3(reflected.x, -reflected.y, reflected.z);

    vec4 reflectedEnvironment = texture(env_kd, correctedReflected);

    float diffuse = max(dot(normalize(normals), lightDir),0.01);
    float spec = 0.0;

    if ( diffuse > 0.0){
        spec = pow(max(dot(normalize(normals), halfVec),0.0),alpha);
    }

    vec4 clr_kd = texture(tex_kd, textureCoord); // Diffuse texture
    vec4 clr_ks = texture(tex_ks, textureCoord); // Specular texture

    vec3 Kd = clr_kd.rgb;
    vec3 Ks = clr_ks.rgb;
    float trans = clr_kd.a;
    
    vec4 color = vec4((Kd * diffuse) + (Ks*spec),trans) + vec4(ambient, 0.4);
    
    fragColor = mix(color, reflectedEnvironment, 0.6);
    //fragColor = reflectedEnvironment;
    //fragColor = vec4(1.0,1.0,1.0,1.0);
}
