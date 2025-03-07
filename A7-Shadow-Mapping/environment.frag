layout(location=0) out vec4 fragColor;
in vec3 dir;

uniform samplerCube cubemap;

void main()
{
    fragColor = texture(cubemap, dir);
}