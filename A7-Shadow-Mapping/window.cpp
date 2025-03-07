// #include <C:\MinGW\GL\freeglut\include\GL\freeglut.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>
#include "cyCodeBase/cyTriMesh.h"
#include "cyCodeBase/cyQuat.h"
#include "cyCodeBase/cyGL.h"
#include "lodepng/lodepng.h"

namespace fs = std::filesystem;

// Global
bool mouse_down;
bool left_button;
bool ctrl_button;
bool alt_opt;

float previous_x, previous_y;

float y_drag = 0.0f, x_drag = 0.0f;
float plane_y_drag = 0.0f, plane_x_drag = 0.0f;
float z_distance = 50.0f, plane_z_distance = 50.0f;

float sensitivity = 0.005;

int num_vertices;
GLfloat light_x = 1.0f;
GLfloat light_y;
GLfloat light_z;

cyGLRenderTexture2D render_buffer;
cyGLRenderDepth2D shadowMap;

GLuint plane_VAO;
GLuint env_VAO;
GLuint light_VAO;
GLuint VAO;

GLuint object_program;
GLuint plane_program;
GLuint light_program;
GLuint env_program;
GLuint shadow_program;

cy::GLSLProgram teapotProgram;
cy::GLSLProgram planeProgram;
cy::GLSLProgram lightProgram;
cy::GLSLProgram envProgram;

GLfloat mvp[16];
GLfloat mv[16];
GLfloat vp[16];
GLfloat model[16];
GLfloat view[16];
GLfloat normal[9];

GLfloat mvp_plane[16];
// GLfloat mv_plane[16];
GLfloat vp_plane[16];
// GLfloat model_plane[16];
// GLfloat view_plane[16];
// GLfloat normal_plane[9];

cy::Matrix4<float> perspective;
cy::Matrix4<float> viewMatrix;

void renderObject();
void setObjectUniform();
void setMatrices();

GLchar *ReadFromFile(const char *file)
{
    std::ifstream inputFile(file);
    if (!inputFile.is_open())
    {
        std::cerr << "Cant open file" << file << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << inputFile.rdbuf();
    inputFile.close();

    // Convert to char*
    std::string file_content = buffer.str();
    GLchar *output = new GLchar[file_content.length() + 1];
    strcpy(output, file_content.c_str());
    return output;
}
void setTextures(cy::TriMesh obj)
{

    cy::TriMesh::Mtl &material = obj.M(0);

    std::vector<unsigned char> image_kd;
    unsigned width_kd, height_kd;

    std::vector<unsigned char> image_ks;
    unsigned width_ks, height_ks;

    if (obj.NM() == 0)
    {
        lodepng::decode(image_kd, width_kd, height_kd, std::string("brick.png"));
        lodepng::decode(image_ks, width_ks, height_ks, std::string("brick-specular.png"));
    }
    else // Get material
    {

        // Decode PNG image from file
        unsigned error = lodepng::decode(image_kd, width_kd, height_kd, std::string(material.map_Kd.data));
        if (error)
        {
            std::cerr << "KD decoder error " << error << ": " << std::endl;
            return;
        }

        unsigned error_ks = lodepng::decode(image_ks, width_ks, height_ks, std::string(material.map_Ks.data));
        if (error_ks)
        {
            std::cerr << "KS decoder error " << error_ks << ": " << std::endl;
            return;
        }
    }

    glActiveTexture(GL_TEXTURE0);

    // Alternative setup
    cyGLTexture2D tex_kd;
    tex_kd.Initialize();
    tex_kd.SetImage(image_kd.data(), 4, width_kd, height_kd);
    tex_kd.BuildMipmaps();
    tex_kd.Bind(0);

    glActiveTexture(GL_TEXTURE1);
    cyGLTexture2D tex_ks;
    tex_ks.Initialize();
    tex_ks.SetImage(image_ks.data(), 4, width_ks, height_ks);
    tex_ks.BuildMipmaps();
    tex_ks.Bind(1);
}
void setObjectVertices(cy::TriMesh obj)
{
    // object_program = LoadShader("shader.vert", "shader.frag");
    teapotProgram.BuildFiles("shader.vert", "shader.frag");
    object_program = teapotProgram.GetID();

    // Vertex array object
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Vertex Buffer Object
    std::vector<GLfloat> positions;
    std::vector<GLfloat> normals;
    std::vector<GLfloat> textures;
    obj.ComputeNormals(false);

    for (unsigned int i = 0; i < obj.NF(); ++i)
    {
        cy::TriMesh::TriFace &face = obj.F(i);
        cy::Vec3f &vertex1 = obj.V(face.v[0]);
        cy::Vec3f &vertex2 = obj.V(face.v[1]);
        cy::Vec3f &vertex3 = obj.V(face.v[2]);

        positions.push_back(vertex1.x);
        positions.push_back(vertex1.y);
        positions.push_back(vertex1.z);
        positions.push_back(vertex2.x);
        positions.push_back(vertex2.y);
        positions.push_back(vertex2.z);
        positions.push_back(vertex3.x);
        positions.push_back(vertex3.y);
        positions.push_back(vertex3.z);

        // Vertex Normals
        cy::Vec3f &normal1 = obj.VN(face.v[0]);
        cy::Vec3f &normal2 = obj.VN(face.v[1]);
        cy::Vec3f &normal3 = obj.VN(face.v[2]);

        normals.push_back(normal1.x);
        normals.push_back(normal1.y);
        normals.push_back(normal1.z);
        normals.push_back(normal2.x);
        normals.push_back(normal2.y);
        normals.push_back(normal2.z);
        normals.push_back(normal3.x);
        normals.push_back(normal3.y);
        normals.push_back(normal3.z);

        // TEXTURE COORDINATES
        cy::TriMesh::TriFace &texface = obj.FT(i);
        cy::Vec3f &tex1 = obj.VT(texface.v[0]);
        cy::Vec3f &tex2 = obj.VT(texface.v[1]);
        cy::Vec3f &tex3 = obj.VT(texface.v[2]);

        textures.push_back(tex1.x);
        textures.push_back(tex1.y);
        textures.push_back(tex2.x);
        textures.push_back(tex2.y);
        textures.push_back(tex3.x);
        textures.push_back(tex3.y);
    }
    num_vertices = obj.NF() * 3;

    // POSITIONS BUFFER
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer); // Bind something, all buffer operations will use this buffer
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat),
                 positions.data(),
                 GL_STATIC_DRAW); // Not planning to modify data frequently
    GLuint pos = glGetAttribLocation(object_program, "pos");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer( // How to interpret data
        pos, 3, GL_FLOAT,  // Will use previously binded buffer, 3d vector of floats
        GL_FALSE, 0, (GLvoid *)0);

    // NORMAL BUFFER
    GLuint normalBuffer;
    glGenBuffers(1, &normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffer); // Bind something, all buffer operations will use this buffer
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat),
                 normals.data(),
                 GL_STATIC_DRAW); // Not planning to modify data frequently
    GLuint normal = glGetAttribLocation(object_program, "normal");
    glEnableVertexAttribArray(normal);
    glVertexAttribPointer(   // How to interpret data
        normal, 3, GL_FLOAT, // Will use previously binded buffer, 3d vector of floats
        GL_FALSE, 0, (GLvoid *)0);

    // TEXTURE BUFFER
    GLuint textureBuffer;
    glGenBuffers(1, &textureBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, textureBuffer); // Bind something, all buffer operations will use this buffer
    glBufferData(GL_ARRAY_BUFFER, textures.size() * sizeof(GLfloat),
                 textures.data(),
                 GL_STATIC_DRAW); // Not planning to modify data frequently
    GLuint tex = glGetAttribLocation(object_program, "txc");
    glEnableVertexAttribArray(tex);
    glVertexAttribPointer( // How to interpret data
        tex, 2, GL_FLOAT,  // Will use previously binded buffer, 3d vector of floats
        GL_FALSE, 0, (GLvoid *)0);

    // Set final texture with render buffer
    // renderObject();
    glUseProgram(0);
}
void setLightVertices(cy::TriMesh obj)
{
    // object_program = LoadShader("shader.vert", "shader.frag");
    lightProgram.BuildFiles("light.vert", "light.frag");
    light_program = lightProgram.GetID();

    // Vertex array object
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Vertex Buffer Object
    std::vector<GLfloat> positions;
    std::vector<GLfloat> normals;
    std::vector<GLfloat> textures;
    obj.ComputeNormals(false);

    for (unsigned int i = 0; i < obj.NF(); ++i)
    {
        cy::TriMesh::TriFace &face = obj.F(i);
        cy::Vec3f &vertex1 = obj.V(face.v[0]);
        cy::Vec3f &vertex2 = obj.V(face.v[1]);
        cy::Vec3f &vertex3 = obj.V(face.v[2]);

        positions.push_back(vertex1.x);
        positions.push_back(vertex1.y);
        positions.push_back(vertex1.z);
        positions.push_back(vertex2.x);
        positions.push_back(vertex2.y);
        positions.push_back(vertex2.z);
        positions.push_back(vertex3.x);
        positions.push_back(vertex3.y);
        positions.push_back(vertex3.z);

        // Vertex Normals
        cy::Vec3f &normal1 = obj.VN(face.v[0]);
        cy::Vec3f &normal2 = obj.VN(face.v[1]);
        cy::Vec3f &normal3 = obj.VN(face.v[2]);

        normals.push_back(normal1.x);
        normals.push_back(normal1.y);
        normals.push_back(normal1.z);
        normals.push_back(normal2.x);
        normals.push_back(normal2.y);
        normals.push_back(normal2.z);
        normals.push_back(normal3.x);
        normals.push_back(normal3.y);
        normals.push_back(normal3.z);

        // TEXTURE COORDINATES
        cy::TriMesh::TriFace &texface = obj.FT(i);
        cy::Vec3f &tex1 = obj.VT(texface.v[0]);
        cy::Vec3f &tex2 = obj.VT(texface.v[1]);
        cy::Vec3f &tex3 = obj.VT(texface.v[2]);

        textures.push_back(tex1.x);
        textures.push_back(tex1.y);
        textures.push_back(tex2.x);
        textures.push_back(tex2.y);
        textures.push_back(tex3.x);
        textures.push_back(tex3.y);
    }
    num_vertices = obj.NF() * 3;

    // POSITIONS BUFFER
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer); // Bind something, all buffer operations will use this buffer
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat),
                 positions.data(),
                 GL_STATIC_DRAW); // Not planning to modify data frequently
    GLuint pos = glGetAttribLocation(object_program, "pos");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer( // How to interpret data
        pos, 3, GL_FLOAT,  // Will use previously binded buffer, 3d vector of floats
        GL_FALSE, 0, (GLvoid *)0);

    // NORMAL BUFFER
    GLuint normalBuffer;
    glGenBuffers(1, &normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffer); // Bind something, all buffer operations will use this buffer
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GLfloat),
                 normals.data(),
                 GL_STATIC_DRAW); // Not planning to modify data frequently
    GLuint normal = glGetAttribLocation(object_program, "normal");
    glEnableVertexAttribArray(normal);
    glVertexAttribPointer(   // How to interpret data
        normal, 3, GL_FLOAT, // Will use previously binded buffer, 3d vector of floats
        GL_FALSE, 0, (GLvoid *)0);

    // TEXTURE BUFFER
    GLuint textureBuffer;
    glGenBuffers(1, &textureBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, textureBuffer); // Bind something, all buffer operations will use this buffer
    glBufferData(GL_ARRAY_BUFFER, textures.size() * sizeof(GLfloat),
                 textures.data(),
                 GL_STATIC_DRAW); // Not planning to modify data frequently
    GLuint tex = glGetAttribLocation(object_program, "txc");
    glEnableVertexAttribArray(tex);
    glVertexAttribPointer( // How to interpret data
        tex, 2, GL_FLOAT,  // Will use previously binded buffer, 3d vector of floats
        GL_FALSE, 0, (GLvoid *)0);

    // Set final texture with render buffer
    // renderObject();
    glUseProgram(0);
}
void setCubeMap()
{
    envProgram.BuildFiles("environment.vert", "environment.frag");
    env_program = envProgram.GetID();

    cy::GLTextureCubeMap envmap;
    envmap.Initialize();
    int i = 0;
    for (const auto &entry : fs::directory_iterator("church"))
    {
        std::cout << entry << std::endl;
        // Load pngs
        std::vector<unsigned char> image;
        unsigned width, height;

        // Decode PNG image from file
        unsigned error = lodepng::decode(image, width, height, std::string(entry.path().string()));
        if (error)
        {
            std::cerr << "Image decoder error " << error << ": " << std::endl;
            return;
        }
        // Set image data
        envmap.SetImageRGBA(
            (cy::GLTextureCubeMap::Side)i++,
            image.data(), width, height);
    }
    envmap.BuildMipmaps();
    envmap.SetSeamless(); // Avoid obvious edges
    envmap.Bind(3);       // Bind to texture unit 3

    // Create big triangle of [-1,-1],[3,-1],[-1,3]
    GLfloat env_vertices[] = {
        // Triangle 1
        -1.0f, -1.0f, 0.0f,
        3.0f, -1.0f, 0.0f,
        -1.0f, 3.0f, 0.0f};

    glGenVertexArrays(1, &env_VAO);
    glBindVertexArray(env_VAO);

    // Positions
    GLuint envBuffer;
    glGenBuffers(1, &envBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, envBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(env_vertices), env_vertices, GL_STATIC_DRAW);

    GLuint envPos = glGetAttribLocation(env_program, "env_positions");
    glEnableVertexAttribArray(envPos);
    glVertexAttribPointer(envPos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
    glBindVertexArray(0);
}
cy::Matrix4<float> getRotation(float x_drag, float y_drag)
{
    // TRANSFORMATION MATRIX
    cy::Matrix3f yRotation;
    yRotation.SetRotationY(x_drag);

    cy::Matrix3f xRotation;
    xRotation.SetRotationX(y_drag);

    cy::Matrix4<float> yRotation4(yRotation);
    cy::Matrix4<float> xRotation4(xRotation);
    return xRotation4 * yRotation4;
}
void initRenderPlane()
{
    // render_buffer.Initialize(
    //     true, // Do you need a depth buffer, creates depth buffer
    //     3,    // RGB
    //     700,
    //     700);
    // plane_program = LoadShader("plane.vert", "plane.frag");
    planeProgram.BuildFiles("plane.vert", "plane.frag");
    plane_program = planeProgram.GetID();

    glGenVertexArrays(1, &plane_VAO);
    glBindVertexArray(plane_VAO);

    GLfloat plane_vertices[] = {
        // Triangle 1
        -20.0f, 0.0f, 20.0f,
        -20.0f, 0.0f, -20.0f,
        20.0f, 0.0f, -20.0f,
        // Triangle 2
        -20.0f, 0.0f, 20.0f,
        20.0f, 0.0f, -20.0f,
        20.0f, 0.0f, 20.0f};

    GLfloat texCoords[] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,

        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f};

    // Positions
    GLuint plane_buffer;
    glGenBuffers(1, &plane_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, plane_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vertices), plane_vertices, GL_STATIC_DRAW);

    GLuint plane_pos = glGetAttribLocation(plane_program, "plane_pos");
    glEnableVertexAttribArray(plane_pos);
    glVertexAttribPointer(plane_pos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);

    // Texture Coords
    GLuint plane_tex_buffer;
    glGenBuffers(1, &plane_tex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, plane_tex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);

    GLuint plane_tex_coords = glGetAttribLocation(plane_program, "plane_tex_coords");
    glEnableVertexAttribArray(plane_tex_coords);
    glVertexAttribPointer(plane_tex_coords, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)0);

    glBindVertexArray(0); // Unbind
    glUseProgram(0);
}
void init(cy::TriMesh obj)
{
    perspective.SetPerspective(1.0472f, 1.0f, 0.1f, 1000.0f);
    viewMatrix.SetView(cy::Vec3f(0.0f, 0.0f, 0.0f), cy::Vec3f(0.0f, 0.0f, 0.0f), cy::Vec3f(0.0f, 1.0f, 0.0f));

    (perspective * viewMatrix * getRotation(plane_x_drag, plane_y_drag)).GetInverse().Get(vp_plane);

    setTextures(obj);
    setObjectVertices(obj);
    setMatrices();
    setObjectUniform();
    setCubeMap();
}
void renderDepthMap()
{
    // Just for rendering the shadow
    shadow_program = glCreateProgram(); // Fragment shader gets thrown out, not used at all

    // Frame buffer
    shadowMap.Initialize(
        true, // Do you need a depth buffer, creates depth buffer
        700,
        700); // Use default depth component

    shadowMap.SetTextureFilteringMode(GL_LINEAR, GL_LINEAR);
    shadowMap.Bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(shadow_program);
    // glUniformMatrix4fv(glGetUniformLocation(shadow_program, "mvp"), 1, GL_FALSE, mlp);
    // Create depth texture
    // Configure frame buffer
    // glDrawArrays(...);
    shadowMap.Unbind();

    // When creating the matrixshadow (T * S(0.5 uniform) * M(mlp)) add bias to T (0.5,0.5,0.5 - bias)
}
void setShadowMatrices()
{
}
void setMatrices()
{
    cy::Matrix4<float> rotation = getRotation(x_drag, y_drag);
    cy::Matrix4<float> rotation_plane = getRotation(plane_x_drag, plane_y_drag);

    cy::Matrix4<float> viewMatrix;
    // Use the calculated camera position
    viewMatrix.SetView(cy::Vec3f(0.0f, 0.0f, z_distance), cy::Vec3f(0.0f, 0.0f, 0.0f), cy::Vec3f(0.0f, 1.0f, 0.0f));

    // Look at the object from the perspective of the light:
    cy::Matrix4<float> lightMatrix;
    lightMatrix.SetView(cy::Vec3f(light_x, light_y, light_y), cy::Vec3f(0.0f, 0.0f, 0.0f), cy::Vec3f(0.0f, 1.0f, 0.0f));

    cy::Matrix4<float> modelMatrix = rotation_plane;
    cy::Matrix4<float> modelView = viewMatrix * rotation * modelMatrix;
    cy::Matrix4<float> modelViewProjection = perspective * modelView;
    cy::Matrix4<float> modelLightProjection = perspective * lightMatrix * modelMatrix;
    cy::Matrix3<float> normalMatrix = modelView.GetSubMatrix3().GetTranspose().GetInverse();
    cy::Matrix4<float> viewProjection = perspective * rotation;

    modelViewProjection.Get(mvp);
    viewProjection.GetInverse().Get(vp);
    normalMatrix.Get(normal);
    modelView.Get(mv);
    modelMatrix.Get(model);
    viewMatrix.Get(view);

    glUseProgram(object_program);

    glUniformMatrix4fv(glGetUniformLocation(object_program, "mvp"), 1, GL_FALSE, mvp);
    glUniformMatrix4fv(glGetUniformLocation(object_program, "mv"), 1, GL_FALSE, mv);
    glUniformMatrix4fv(glGetUniformLocation(object_program, "view"), 1, GL_FALSE, view);
    glUniformMatrix3fv(glGetUniformLocation(object_program, "mNorm"), 1, GL_FALSE, normal);
    glUniformMatrix3fv(glGetUniformLocation(object_program, "model"), 1, GL_FALSE, model);

    GLint lightPosLoc = glGetUniformLocation(object_program, "lightPos");
    GLint cameraLoc = glGetUniformLocation(object_program, "camera");
    glUniform3f(lightPosLoc, light_x, light_y, light_z);
    glUniform3f(cameraLoc, view[12], view[13], view[14]);

    glUseProgram(0);

    // Convert to Light space World -> Light space ->Canonical view volume (Model-Light-Projection Matrix)
    // Two canonical view volume
}
void setObjectUniform()
{
    glUseProgram(object_program);

    GLint alphaLoc = glGetUniformLocation(object_program, "alpha");
    GLint ambientLoc = glGetUniformLocation(object_program, "ambient");
    GLint sampler_kd = glGetUniformLocation(object_program, "tex_kd");
    GLint sampler_ks = glGetUniformLocation(object_program, "tex_ks");
    GLint sampler_env = glGetUniformLocation(object_program, "env_kd");

    glUniform1f(alphaLoc, 60.1f);
    glUniform3f(ambientLoc, 0.0f, 0.07f, 0.09f);
    glUniform1i(sampler_kd, 0);  // Get texture from texture unit
    glUniform1i(sampler_ks, 1);  // Get texture from texture unit
    glUniform1i(sampler_env, 3); // Get texture from texture unit

    glUseProgram(0);
}
void renderObject()
{
    glUseProgram(object_program);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES,
                 0,
                 num_vertices);
    glBindVertexArray(0);
    glUseProgram(0);
}
void display()
{
    // Get the rendered object
    // render_buffer.Bind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw CubeMap
    glDepthMask(GL_FALSE);

    glUseProgram(env_program);

    glUniformMatrix4fv(glGetUniformLocation(env_program, "vp"), 1, GL_FALSE, vp);

    glBindVertexArray(env_VAO);
    glActiveTexture(GL_TEXTURE3);
    glUniform1i(glGetUniformLocation(env_program, "cubemap"), 3); // Render previously loaded object as a texture and set to texture unit 0

    // Draw one big triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glUseProgram(0);
    glDepthMask(GL_TRUE);

    // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // // !RENDERING!
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    renderObject(); // Bind and unbind and draw obj
    // render_buffer.Unbind();

    // // Draw Plane
    // glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glUseProgram(plane_program);

    GLint planeMvpLoc = glGetUniformLocation(plane_program, "mvp");
    glUniformMatrix4fv(planeMvpLoc, 1, GL_FALSE, mvp);
    // glUniform1i(glGetUniformLocation(plane_program, "renderedTexture"), 2); // Render previously loaded object as a texture and set to texture unit 0

    glBindVertexArray(plane_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6); // Draw the plane
    glBindVertexArray(0);

    glDepthMask(GL_FALSE);    // Disable depth writing
    glDisable(GL_DEPTH_TEST); // Disable depth testing

    glutSwapBuffers();
}
void keyboard(unsigned char key, int x, int y)
{
    int mod = glutGetModifiers();
    ctrl_button = (mod & GLUT_ACTIVE_CTRL);
    alt_opt = (mod & GLUT_ACTIVE_ALT);
    // Handle keyboard input
    switch (key)
    {
    case 27: // ECS
        /* code */
        glutLeaveMainLoop();
        break;
    case 'r':
        light_x = 1.0f;
        light_z = 1.0f;
        x_drag = 0.0f;
        y_drag = 0.0f;
        plane_x_drag = 0.0f;
        plane_y_drag = 0.0f;
        setMatrices();
        break;

    default:
        break;
    }
    glutPostRedisplay();
}
void specialKeys(int key, int x, int y)
{
    int mod = glutGetModifiers();
    ctrl_button = (mod & GLUT_ACTIVE_CTRL);
    alt_opt = (mod & GLUT_ACTIVE_ALT);

    switch (key)
    {
    default:
        break;
    }
    glutPostRedisplay();
}
void keyboard2(int key, int x, int y)
{
    // Handle keyboard input
}
void mouse(int button, int state, int x, int y)
{
    // Left mouse: Rotate
    // right mouse: Distance
    mouse_down = (state == GLUT_DOWN);
    left_button = (button == GLUT_LEFT_BUTTON);
    int mod = glutGetModifiers();
    ctrl_button = (mod & GLUT_ACTIVE_CTRL);
    alt_opt = (mod & GLUT_ACTIVE_ALT);
}
void mouseMotion(int x, int y)
{
    // Handle mouse input
    if (mouse_down)
    {
        if (left_button)
        {
            if (ctrl_button)
            {
                light_x += (x - previous_x) * 0.5f;
                light_y -= (y - previous_y) * 0.5f;
            }
            else if (alt_opt)
            {
                plane_x_drag += (x - previous_x) * sensitivity;
                plane_y_drag += (y - previous_y) * sensitivity;

                // Cubemap rotation change
                (perspective * viewMatrix * getRotation(plane_x_drag, plane_y_drag)).GetInverse().Get(vp_plane);
            }
            else
            {
                x_drag += (x - previous_x) * sensitivity;
                y_drag += (y - previous_y) * sensitivity;
            }
        }
        if (!left_button)
        {
            if (ctrl_button)
            {
                light_z += (x - previous_x);
            }
            else if (alt_opt)
            {
                plane_z_distance += (x - previous_x);
            }
            else
            {
                z_distance += (x - previous_x);
            }
        }
        setMatrices();
        glutPostRedisplay();
    }
    previous_x = x;
    previous_y = y;
}

void mousePassive(int x, int y)
{
    // While button not down
}
void reshape(int x, int y)
{
    // When window size changes
}
void idle()
{
    glutPostRedisplay(); // Do not call directly
}

int main(int argc, char **argv)
{
    // Init
    glutInit(&argc, argv);

    // Create Window
    glutInitWindowSize(700, 700);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutCreateWindow("A5 - Render Buffers");

    // Initialize GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
        return -1;
    }

    std::string filepath;
    if (argc > 1)
    {
        filepath = argv[1];
    }
    else
    {
        std::cout << "Enter the file location of your .obj file: ";
        std::getline(std::cin, filepath);
    }

    bool ObjectLoaded;
    cy::TriMesh obj;
    bool status = obj.LoadFromFileObj(filepath.c_str(), true, &std::cout);
    std::cout << status << filepath << std::endl;

    // Send positions and normals
    initRenderPlane(); // Create plane shaders
    init(obj);         // Creat obj shaders

    // glUseProgram(object_program);

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    // glutSpecialFunc(keyboard2);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(mouseMotion);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);

    glClearColor(0, 0, 0, 1);

    // Call main loop
    glutMainLoop();
    return 0;
}
