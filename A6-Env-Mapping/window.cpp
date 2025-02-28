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

// Global
bool mouse_down;
bool left_button;
bool ctrl_button;
bool alt_opt;

float previous_x, previous_y;

float y_drag = 0.0f, x_drag = 0.0f;
float plane_y_drag = 0.0f, plane_x_drag = 0.0f;

float sensitivity = 0.005;

float z_distance = 50.0f, plane_z_distance = 50.0f;
int num_vertices;
GLfloat light_x = 1.0f;
GLfloat light_y;
GLfloat light_z;

cyGLRenderTexture2D render_buffer;
GLsizei textureWidth;
GLsizei textureHeight;
GLuint plane_VAO;
GLuint VAO;

GLuint program;
GLuint plane_program;
cy::GLSLProgram teapotProg;
cy::GLSLProgram planeProg;

GLfloat mvp[16];
GLfloat mv[16];
GLfloat model[16];
GLfloat Nmatrix[9];

GLfloat mvp_plane[16];
GLfloat mv_plane[16];
GLfloat model_plane[16];
GLfloat Nmatrix_plane[9];

void renderToTexture();
namespace fs = std::filesystem;

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

    std::cout << material.map_Ka << std::endl;

    std::vector<unsigned char> image_kd;
    unsigned width_kd, height_kd;

    // Decode PNG image from file
    unsigned error = lodepng::decode(image_kd, width_kd, height_kd, std::string(material.map_Kd.data));
    if (error)
    {
        std::cerr << "KD decoder error " << error << ": " << std::endl;
        return;
    }

    std::vector<unsigned char> image_ks;
    unsigned width_ks, height_ks;
    unsigned error_ks = lodepng::decode(image_ks, width_ks, height_ks, std::string(material.map_Ks.data));
    if (error_ks)
    {
        std::cerr << "KS decoder error " << error_ks << ": " << std::endl;
        return;
    }

    textureWidth = width_kd;
    textureHeight = height_kd;

    std::cout << textureHeight << std::endl;

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
void setCubeMap()
{
    cy::GLTextureCubeMap envmap;
    envmap.Initialize();
    int i = 0;
    for (const auto &entry : fs::directory_iterator("cubemap"))
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
    envmap.Bind(0);       // Bind to texture unit 0
}
void setMatrices(GLfloat mvp[16],
                 GLfloat mv[16],
                 GLfloat model[16],
                 GLfloat Nmatrix[9],
                 float x_drag, float y_drag, float z_distance)
{
    // TRANSFORMATION MATRIX
    cy::Matrix3f xRotation;
    xRotation.SetRotationX(x_drag);
    cy::Matrix3f zRotation;
    zRotation.SetRotationZ(y_drag);

    cy::Matrix4<float> xRotation4(xRotation);
    cy::Matrix4<float> zRotation4(zRotation);

    // PERSPECTIVE MATRIX
    cy::Matrix4<float> perspective;
    perspective.SetPerspective(1.0472f, 1.0f, 0.1f, 1000.0f);

    cy::Matrix4<float> translation;
    translation.SetTranslation(cy::Vec3f(0.0f, -5.0f, -z_distance));

    cy::Matrix4<float> transformation = translation * xRotation4 * zRotation4;
    cy::Matrix4<float> viewMatrix;

    viewMatrix.SetView(cy::Vec3f(0.0f, 0.0f, 10.0f), cy::Vec3f(0.0f, 0.0f, 0.0f), cy::Vec3f(0.0f, 1.0f, 0.0f));
    float viewMatrixData[16];
    viewMatrix.Get(viewMatrixData);

    cy::Matrix4<float> modelView = viewMatrix * transformation;
    cy::Matrix4<float> modelViewPerspective = perspective * modelView;
    cy::Matrix3<float> normalMatrix = modelView.GetSubMatrix3().GetTranspose().GetInverse();

    modelViewPerspective.Get(mvp);
    normalMatrix.Get(Nmatrix);
    modelView.Get(mv);
    transformation.Get(model);
}
void initRenderPlane()
{
    // plane_program = LoadShader("plane.vert", "plane.frag");
    planeProg.BuildFiles("plane.vert", "plane.frag");
    plane_program = planeProg.GetID();

    glGenVertexArrays(1, &plane_VAO);
    glBindVertexArray(plane_VAO);

    GLfloat plane_vertices[] = {
        // Triangle 1
        -10.0f, 10.0f,
        -10.0f, -10.0f,
        10.0f, -10.0f,
        // Triangle 2
        -10.0f, 10.0f,
        10.0f, -10.0f,
        10.0f, 10.0f};

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
    glVertexAttribPointer(plane_pos, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid *)0);

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
    setMatrices(mvp, mv, model, Nmatrix, x_drag, y_drag, z_distance);
    setMatrices(mvp_plane, mv_plane, model_plane, Nmatrix_plane, plane_x_drag, plane_y_drag, plane_z_distance);

    setTextures(obj);
    setCubeMap();

    // program = LoadShader("shader.vert", "shader.frag");
    teapotProg.BuildFiles("shader.vert", "shader.frag");
    program = teapotProg.GetID();

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
    // renderToTexture();
    glUseProgram(0);
}
void renderToTexture()
{
    render_buffer.Bind();
    // glViewport(0, 0, textureWidth, textureHeight);

    // if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    // {
    //     std::cerr << "Framebuffer is not complete!" << std::endl;
    //     render_buffer.Unbind();
    //     return;
    // }

    // Set uniform
    glUseProgram(program);

    GLint mvpLoc = glGetUniformLocation(program, "mvp");
    GLint mvLoc = glGetUniformLocation(program, "mv");
    GLint mNormLoc = glGetUniformLocation(program, "mNorm");
    GLint modelLoc = glGetUniformLocation(program, "model");

    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp);
    glUniformMatrix4fv(mvLoc, 1, GL_FALSE, mv);
    glUniformMatrix3fv(mNormLoc, 1, GL_FALSE, Nmatrix);
    glUniformMatrix3fv(modelLoc, 1, GL_FALSE, model);

    // Fragment Shader:
    GLint lightPosLoc = glGetUniformLocation(program, "lightPos");
    GLint alphaLoc = glGetUniformLocation(program, "alpha");
    GLint ambientLoc = glGetUniformLocation(program, "ambient");
    GLint sampler_kd = glGetUniformLocation(program, "tex_kd");
    GLint sampler_ks = glGetUniformLocation(program, "tex_ks");

    glUniform3f(lightPosLoc, light_x, light_y, light_z);
    glUniform1f(alphaLoc, 60.1f);
    glUniform3f(ambientLoc, 0.0f, 0.07f, 0.09f);
    glUniform1i(sampler_kd, 0); // Get texture from texture unit
    glUniform1i(sampler_ks, 1); // Get texture from texture unit

    // // !RENDERING!
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES,
                 0,
                 num_vertices);
    glBindVertexArray(0);
    glUseProgram(0);

    render_buffer.Unbind();
}
void display()
{

    // Get the rendered object
    renderToTexture(); // Bind and unbind and draw obj

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glActiveTexture(GL_TEXTURE2);
    render_buffer.BindTexture(2);

    glUseProgram(plane_program);

    GLint planeMvpLoc = glGetUniformLocation(plane_program, "mvp");
    glUniformMatrix4fv(planeMvpLoc, 1, GL_FALSE, mvp_plane);
    glUniform1i(glGetUniformLocation(plane_program, "renderedTexture"), 2); // Render previously loaded object as a texture and set to texture unit 0

    glBindVertexArray(plane_VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6); // Draw the plane
    glBindVertexArray(0);

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
                plane_y_drag += (x - previous_x) * sensitivity;
                plane_x_drag += (y - previous_y) * sensitivity;
            }
            else
            {
                y_drag += (x - previous_x) * sensitivity;
                x_drag += (y - previous_y) * sensitivity;
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

        setMatrices(mvp, mv, model, Nmatrix, x_drag, y_drag, z_distance);
        setMatrices(mvp_plane, mv_plane, model_plane, Nmatrix_plane, plane_x_drag, plane_y_drag, plane_z_distance);
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

    // glUseProgram(program);

    render_buffer.Initialize(
        true, // Do you need a depth buffer, creates depth buffer
        3,    // RGB
        700,
        700);

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
