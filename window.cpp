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
#include "cyCodeBase/cyTriMesh.h"
#include "cyCodeBase/cyQuat.h"
// #include "C:/msys64/mingw64/include/GL/freeglut.h"

// Global
cy::TriMesh obj;
bool mouse_down;
bool left_button;
float previous_x, previous_y;
float y_drag = 0.0f, x_drag = 0.0f;
float sensitivity = 0.01;
float scalar = 0.1;

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
GLuint LoadShader(const char *vs_path, const char *fs_path)
{
    // SHADERS
    GLuint program = glCreateProgram();

    // Compile Vertex shader //

    GLchar *vsSource = ReadFromFile(vs_path);
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);

    glShaderSource(vs, 1, &vsSource, nullptr);
    glCompileShader(vs);
    // Check if compilation is successfull
    int compiled;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLsizei len;
        glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &len);
        std::vector<GLchar> log(len);
        glGetShaderInfoLog(vs, len, &len, log.data());
        std::cerr << "Vertex shader compilation failed: " << log.data() << std::endl;
        delete[] vsSource;
        return 0;
    }
    glAttachShader(program, vs);
    delete[] vsSource;

    // Compile Fragment Shader //

    GLchar *fsSource = ReadFromFile(fs_path);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSource, nullptr);
    glCompileShader(fs);
    // Check if compilation is successfull
    glGetShaderiv(fs, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLsizei len;
        glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &len);
        std::vector<GLchar> log(len);
        glGetShaderInfoLog(fs, len, &len, log.data());
        std::cerr << "Fragment shader compilation failed: " << log.data() << std::endl;
        delete[] fsSource;
        return 0;
    }
    glAttachShader(program, fs);
    delete[] fsSource;

    glLinkProgram(program);

    return program;
}

void display()
{
    // INIT //
    //      //

    GLuint program = LoadShader("A2\\shader.vert", "A2\\shader.frag");

    // Vertex array object
    GLuint vertex_array_obj;
    glGenVertexArrays(1, &vertex_array_obj);
    glBindVertexArray(vertex_array_obj);

    // Vertex Buffer Object
    std::vector<GLfloat> positions;

    for (int i = 0; i < obj.NV(); i++)
    {
        const cy::Vec3f &vertex = obj.V(i);
        // std::cout << "Vertex " << i << ": " << vertex.x << ", " << vertex.y << ", " << vertex.z << std::endl;
        positions.push_back(static_cast<GLfloat>(vertex.x));
        positions.push_back(static_cast<GLfloat>(vertex.y));
        positions.push_back(static_cast<GLfloat>(vertex.z));
    }

    int num_vertices = obj.NV();
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer); // Bind something, all buffer operations will use this buffer
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat),
                 positions.data(),
                 GL_STATIC_DRAW); // Not planning to modify data frequently
    //
    GLuint pos = glGetAttribLocation(program, "pos");
    glEnableVertexAttribArray(pos);
    glVertexAttribPointer( // How to interpret data
        pos, 3, GL_FLOAT,  // Will use previously binded buffer, 3d vector of floats
        GL_FALSE, 0, (GLvoid *)0);

    // Check for linker errors

    // !RENDERING!
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // OPENGL CALLS

    glUseProgram(program);
    GLint mvpLoc = glGetUniformLocation(program, "mvp");

    cy::Matrix3<float> scalarMat;
    scalarMat.SetScale(scalar);
    cy::Matrix3f xRotation;
    xRotation.SetRotationX(x_drag);
    cy::Matrix3f zRotation;
    zRotation.SetRotationZ(y_drag);

    cy::Matrix4<float> scalarMat4(scalarMat);
    cy::Matrix4<float> xRotation4(xRotation);
    cy::Matrix4<float> zRotation4(zRotation);

    cy::Matrix4<float> transformation = scalarMat4 * xRotation4 * zRotation4;

    float matrixData1[16];
    transformation.Get(matrixData1);

    const GLfloat *matrix = matrixData1;

    // Print the matrix values
    // std::cout << "Matrix 1" << std::endl;
    // for (int i = 0; i < 4; ++i)
    // {
    //     for (int j = 0; j < 4; ++j)
    //     {
    //         std::cout << matrixData1[i * 4 + j] << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // GLfloat matrix[16] = {
    //     scalar, 0.0, 0.0, 0.0,
    //     0.0, scalar, 0.0, 0.0,
    //     0.0, 0.0, scalar, 0.0,
    //     0.0, 0.0, 0.0, 1.0};

    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, matrix);

    // Set point size
    glPointSize(2.0f);

    glDrawArrays(GL_POINTS,
                 0,
                 num_vertices);

    glutSwapBuffers();
}
void keyboard(unsigned char key, int x, int y)
{
    // Handle keyboard input
    switch (key)
    {
    case 27: // ECS
        /* code */
        glutLeaveMainLoop();
        break;

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
    std::cout << left_button << std::endl;
}
void mouseMotion(int x, int y)
{
    // Handle mouse input
    if (mouse_down && left_button)
    {
        y_drag += (x - previous_x) * sensitivity;
        x_drag += (y - previous_y) * sensitivity;
        std::cout << "Mouse drPag left" << std::endl;
    }
    if (mouse_down && !left_button)
    {
        scalar += (x - previous_x) * sensitivity;
        std::cout << "Mouse drag right" << std::endl;
    }

    previous_x = x;
    previous_y = y;
}
void mousePassive(int x, int y)
{
    // Handle mouse input
    // While button not down
}
void reshape(int x, int y)
{
    // When window size changes
}
void idle()
{
    // handle animations
    // glClearColor(red, 0, 0, 0);
    // red += 0.02;

    // Figure out how much time has passed
    // glutGet(GLUT_ELAPSED_TIME)

    // Tell GLUT to redraw
    glutPostRedisplay(); // Do not call directly
}

int main(int argc, char **argv)
{
    // Init
    glutInit(&argc, argv);

    // Create Window
    glutInitWindowSize(600, 700);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
    glutCreateWindow("Hello World");

    // Initialize GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
        return -1;
    }
    std::cout << "File: " << argv[1] << std::endl;
    bool ObjectLoaded;
    bool status = obj.LoadFromFileObj(argv[1], true, &std::cout);
    std::cout << status << std::endl;

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(keyboard2);
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
