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
float sensitivity = 0.005;
float z_distance = 50.0f;

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

    // TRANSFORMATION MATRIX
    cy::Matrix3f xRotation;
    xRotation.SetRotationX(x_drag);
    cy::Matrix3f zRotation;
    zRotation.SetRotationZ(y_drag);

    cy::Matrix4<float> xRotation4(xRotation);
    cy::Matrix4<float> zRotation4(zRotation);

    // PERSPECTIVE MATRIX
    cy::Matrix4<float> perspective;
    perspective.SetPerspective(1.0472f, 1.0f, 0.1f, 100.0f);

    cy::Matrix4<float> translation;
    translation.SetTranslation(cy::Vec3f(0.0f, -5.0f, -z_distance));

    cy::Matrix4<float> transformation = perspective * translation * xRotation4 * zRotation4;

    float matrixData1[16];
    transformation.Get(matrixData1);

    const GLfloat *matrix = matrixData1;

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
}
void mouseMotion(int x, int y)
{
    // Handle mouse input
    if (mouse_down && left_button)
    {
        y_drag += (x - previous_x) * sensitivity;
        x_drag += (y - previous_y) * sensitivity;
    }
    if (mouse_down && !left_button)
    {
        z_distance += (x - previous_x);
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
    glutCreateWindow("A2 - Transformations");

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
    bool status = obj.LoadFromFileObj(filepath.c_str(), true, &std::cout);
    std::cout << status << filepath << std::endl;

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
