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

cy::TriMesh ObjectObj;
cy::TriMesh lightObj;
GLfloat light_x = 10.0f;
GLfloat light_y = 10.0f;
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
cy::GLSLProgram shadowProgram;
cy::GLSLProgram envProgram;

GLfloat mvp[16];
GLfloat mlp[16];
GLfloat matShadow[16];
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

void renderObject(GLuint program, GLuint VAO, int num_vertices);
void setObjectUniform();
void setMatrices();
void setLightMatrices();


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
            
        }
    }

    glActiveTexture(GL_TEXTURE0);
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
void setObjectVertices(cy::TriMesh obj, GLuint program, GLuint VAO)
{
    // object_program = LoadShader("shader.vert", "shader.frag");
    glBindVertexArray(VAO);

    glUseProgram(program);
   
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

    // POSITIONS BUFFER
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer); // Bind something, all buffer operations will use this buffer
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat),
                 positions.data(),
                 GL_STATIC_DRAW); // Not planning to modify data frequently
    GLuint pos = glGetAttribLocation(program, "pos");
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
    GLuint normal = glGetAttribLocation(program, "normal");
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
    GLuint tex = glGetAttribLocation(program, "txc");
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
    envProgram.BuildFiles("shaders/environment.vert", "shaders/environment.frag");
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
    planeProgram.BuildFiles("shaders/plane.vert", "shaders/plane.frag");
    plane_program = planeProgram.GetID();

    glGenVertexArrays(1, &plane_VAO);
    glBindVertexArray(plane_VAO);

    GLfloat plane_vertices[] = {
        // Triangle 1
        -50.0f, 0.0f, 50.0f,
        -50.0f, 0.0f, -50.0f,
        50.0f, 0.0f, -50.0f,
        // Triangle 2
        -50.0f, 0.0f, 50.0f,
        50.0f, 0.0f, -50.0f,
        50.0f, 0.0f, 50.0f};

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
void init()
{
    perspective.SetPerspective(1.0472f, 1.0f, 0.1f, 1000.0f);
    viewMatrix.SetView(cy::Vec3f(0.0f, 0.0f, 0.0f), cy::Vec3f(0.0f, 0.0f, 0.0f), cy::Vec3f(0.0f, 1.0f, 0.0f));

    (perspective * viewMatrix * getRotation(plane_x_drag, plane_y_drag)).GetInverse().Get(vp_plane);

    //Load shaders and VAOs
    teapotProgram.BuildFiles("shaders/shader.vert", "shaders/shader.frag");
    object_program = teapotProgram.GetID();
    glGenVertexArrays(1, &VAO);

    // For light bulb rendering
    lightProgram.BuildFiles("shaders/light.vert", "shaders/light.frag");
    light_program = lightProgram.GetID();
    glGenVertexArrays(1, &light_VAO);


    setObjectVertices(ObjectObj, object_program, VAO);
    setObjectVertices(lightObj, light_program, light_VAO);

    setMatrices();
    setLightMatrices();

    setTextures(ObjectObj);
    setObjectUniform();

    // setTextures(lightObj);
    glUniform1i(glGetUniformLocation(object_program, "renderedTexture"), 0);  // Get texture from texture unit

    setCubeMap();

    //Shadows
    // Render to Depth Texture
    shadowProgram.BuildFiles("shaders/depth.vert", "shaders/depth.frag");
    shadow_program = shadowProgram.GetID();
    // // Frame buffer
    shadowMap.Initialize(
        true, // Do you need a depth buffer, creates depth buffer
        700,
        700); 

    shadowMap.SetTextureFilteringMode(GL_LINEAR, GL_LINEAR);

}

void setLightMatrices()
{
    glUseProgram(light_program);
    cy::Matrix4<float> rotation = getRotation(x_drag, y_drag);
    cy::Vec3f lightPos(light_x, light_y, light_z);
    cy::Vec3f origin(0.0f,0.0f,0.0f);
    // Translation matrix to move the light object to the light's position
    cy::Matrix4<float> translation;
    translation.SetTranslation(lightPos);

    cy::Matrix3<float> rotateToOrigin;
    rotateToOrigin.SetRotation(lightPos.GetNormalized(), (origin - lightPos).GetNormalized());
    cy::Matrix4<float> rotateToOrigin4(rotateToOrigin);



    // Combine rotation and translation to form the final model matrix for the light object
    cy::Matrix4<float> modelMatrix = translation * rotateToOrigin4;

    cy::Matrix4<float> viewMatrix;
    // Use the calculated camera position
    viewMatrix.SetView(cy::Vec3f(0.0f, 0.0f, z_distance), origin, cy::Vec3f(0.0f, 1.0f, 0.0f));
    viewMatrix = viewMatrix * rotation;
    cy::Matrix4<float> modelViewProjection = perspective * viewMatrix * modelMatrix;

    GLfloat mvp_light[16];
    modelViewProjection.Get(mvp_light);
    glUniformMatrix4fv(glGetUniformLocation(light_program, "mvp"), 1, GL_FALSE, mvp_light);

    glUseProgram(0);
}
void setMatrices()
{
    cy::Matrix4<float> rotation = getRotation(x_drag, y_drag);
    cy::Matrix4<float> rotationCamera = getRotation(x_drag, -y_drag);
    cy::Matrix4<float> rotation_plane = getRotation(plane_x_drag, plane_y_drag);

    cy::Vec3f origin(0.0f,0.0f,0.0f);
    // Translation matrix to move the light object to the light's position
      // Update spherical angles based on drag input
      
    // float theta = -y_drag;
    // float phi = -x_drag; // Subtract to invert vertical dragging direction

    // // spherical to cartesian 
    // float camera_x = z_distance * sin(phi) * cos(theta);
    // float camera_y = z_distance * sin(phi) * sin(theta);
    // float camera_z = z_distance * cos(phi);

    // cy::Vec3f camera(camera_x, camera_y, camera_z);

 
    cy::Matrix4<float> translation;
    translation.SetTranslation(cy::Vec3f(0.0f,0.0f,z_distance));
    cy::Matrix4<float> viewMatrix = translation * rotationCamera;
    // cy::Matrix4<float> invViewMatrix = viewMatrix.GetInverse(); // Inverse the view matrix
    cy::Vec3f cameraPosition(viewMatrix.GetInverse().GetTranslation());
    GLfloat camera_coords[3]; // Assuming a 4x4 matrix
    cameraPosition.Get(camera_coords);
    std::cout << " XYZ :" << std::endl;
    std::cout << camera_coords[0] << std::endl;
    std::cout <<  camera_coords[1] << std::endl;
    std::cout <<  camera_coords[2] << std::endl;
    // cy::Matrix4<float> viewMatrix;
    viewMatrix.SetView(cameraPosition, origin, cy::Vec3f(0.0f, 1.0f, 0.0f)); //Camera orbits the origin
    // Look at the object from the perspective of the light:
    cy::Matrix4<float> lightMatrix;
    lightMatrix.SetView(cy::Vec3f(light_x, light_y, light_y), origin, cy::Vec3f(0.0f, 1.0f, 0.0f));

    cy::Matrix4<float> modelMatrix = rotation_plane;
    cy::Matrix4<float> modelView = viewMatrix * modelMatrix;
    cy::Matrix4<float> modelViewProjection = perspective * modelView;
    cy::Matrix4<float> modelLightProjection = perspective * lightMatrix * modelMatrix;
    cy::Matrix3<float> normalMatrix = modelView.GetSubMatrix3().GetInverse().GetTranspose();
    cy::Matrix4<float> viewProjection = perspective * rotation;

    // T * S * MLP
    float bias = 0.001f;
    cy::Matrix4<float> scale;
    scale.SetScale(0.5);
    cy::Matrix4<float> translate;
    scale.SetTranslation(cy::Vec3f(0.5,0.5,0.5 - bias));
    cy::Matrix4<float> matrixShadow =translate * scale * modelLightProjection;

    modelViewProjection.Get(mvp);
    viewProjection.GetInverse().Get(vp);
    normalMatrix.Get(normal);
    modelView.Get(mv);
    modelMatrix.Get(model);
    viewMatrix.Get(view);
    modelLightProjection.Get(mlp); //For depth map
    matrixShadow.Get(matShadow);

    glUseProgram(object_program);

    glUniformMatrix4fv(glGetUniformLocation(object_program, "mvp"), 1, GL_FALSE, mvp);
    glUniformMatrix4fv(glGetUniformLocation(object_program, "view"), 1, GL_FALSE, view);
    glUniformMatrix3fv(glGetUniformLocation(object_program, "mNorm"), 1, GL_FALSE, normal);
    glUniformMatrix3fv(glGetUniformLocation(object_program, "model"), 1, GL_FALSE, model);

    glUniform3f(glGetUniformLocation(object_program, "lightPos"), light_x, light_y, light_z);
    glUniform3f(glGetUniformLocation(object_program, "camera"), camera_coords[0], camera_coords[1], camera_coords[2]);
    glUseProgram(0);

    glUseProgram(shadow_program);
    glUniformMatrix4fv(glGetUniformLocation(shadow_program, "mvp"), 1, GL_FALSE, mlp);
    glUseProgram(0);

    glUseProgram(plane_program);
    glUniformMatrix3fv(glGetUniformLocation(plane_program, "matrixShadow"), 1, GL_FALSE, matShadow);
    glUniformMatrix4fv(glGetUniformLocation(plane_program, "mvp"), 1, GL_FALSE, mvp);
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
void renderObject(GLuint program, GLuint VAO, int num_vertices)
{
    glUseProgram(program);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES,
                 0,
                 num_vertices);
    glBindVertexArray(0);
    glUseProgram(0);
}
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

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
    glClear(GL_DEPTH_BUFFER_BIT);

    //Render the depthmap:
    shadowMap.Bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    renderObject(shadow_program, VAO, ObjectObj.NF() * 3); // Bind and unbind and draw obj
    shadowMap.Unbind();

    // Draw Objects...
    // render_buffer.Bind();
    glActiveTexture(GL_TEXTURE4);
    shadowMap.BindTexture(4);
    glUseProgram(plane_program);
    glUniform1i(glGetUniformLocation(plane_program, "shadow"), 4);

    // renderObject(plane_program, plane_VAO, 6);
    renderObject(object_program, VAO, ObjectObj.NF() * 3); // Bind and unbind and draw obj
    renderObject(light_program, light_VAO, lightObj.NF() * 3); // Bind and unbind and draw obj

    // render_buffer.Unbind();
    glDepthMask(GL_FALSE);    // Disable depth writing

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
        setLightMatrices();
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
        setLightMatrices();

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
    glutCreateWindow("A7 - Shadow Mapping");

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
    bool status = ObjectObj.LoadFromFileObj(filepath.c_str(), true, &std::cout);
    std::cout << status << " " << filepath << std::endl;

    bool lightLoaded;
    bool statusLight = lightObj.LoadFromFileObj("light.obj", true, &std::cout);
    std::cout << statusLight << " light.obj" << std::endl;

    // Send positions and normals
    initRenderPlane(); // Create plane shaders
    init();         // Creat obj shaders

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
