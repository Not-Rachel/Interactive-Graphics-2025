#version 330 core

//Lighting:
uniform vec3 lightPos;
uniform float alpha;

in vec3 normals;
in vec3 posVec;

out vec4 fragColor;
void main(){
    vec3 lightDir = normalize(lightPos);		
    vec3 viewDir = normalize(-posVec);
    vec3 halfVec = normalize(lightDir + viewDir); //Angle between lightDir and viewDir

    float diffuse = max(dot(normals, lightDir),0.0);
    float spec = 0.0;

    if ( diffuse > 0.0){
        spec = pow(max(dot(normals, halfVec),0.0),alpha);
    }
    vec3 Kd = vec3(1.0,1.0,1.0);
    vec3 Ks = vec3(1.0,1.0,1.0);


    float trans = 1.0;
    
    vec4 color = vec4((Kd * diffuse) + (Ks*spec),trans);
    
    fragColor = color;
}
