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
bool mouse_down;
bool left_button;
float previous_x, previous_y;
float y_drag = 0.0f, x_drag = 0.0f;
float sensitivity = 0.005;
float z_distance = 50.0f;
int num_vertices;

GLuint program;
GLfloat mvp[16];
GLfloat mv[16];
GLfloat Nmatrix[9];

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
void setMatrices()
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
    perspective.SetPerspective(1.0472f, 1.0f, 0.1f, 100.0f);

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
}

void init(cy::TriMesh obj)
{
    setMatrices();

    program = LoadShader("shader.vert", "shader.frag");
    // Vertex array object
    GLuint vertex_array_obj;
    glGenVertexArrays(1, &vertex_array_obj);
    glBindVertexArray(vertex_array_obj);

    // Vertex Buffer Object
    std::vector<GLfloat> positions;
    std::vector<GLfloat> normals;
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

        // cy::TriMesh::TriFace &Nface = obj.F(i);
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
}

void display()
{
    // !RENDERING!
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // OPENGL CALLS

    glUseProgram(program);
    GLint mvpLoc = glGetUniformLocation(program, "mvp");
    GLint mvLoc = glGetUniformLocation(program, "mv");
    GLint mNormLoc = glGetUniformLocation(program, "mNorm");

    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp);
    glUniformMatrix4fv(mvLoc, 1, GL_FALSE, mv);
    glUniformMatrix3fv(mNormLoc, 1, GL_FALSE, Nmatrix);

    // Fragment Shader:
    GLint lightPosLoc = glGetUniformLocation(program, "lightPos");
    GLint alphaLoc = glGetUniformLocation(program, "alpha");
    GLint ambientLoc = glGetUniformLocation(program, "ambient");

    glUniform3f(lightPosLoc, 1.0f, 0.0f, 0.0f);
    glUniform1f(alphaLoc, 60.1f);
    glUniform3f(ambientLoc, 0.0f, 0.1f, 0.3f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glDrawArrays(GL_TRIANGLES,
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
    if (mouse_down)
    {
        if (left_button)
        {
            y_drag += (x - previous_x) * sensitivity;
            x_drag += (y - previous_y) * sensitivity;
        }
        if (!left_button)
        {
            z_distance += (x - previous_x);
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
    glutCreateWindow("A3 - Shading");

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
    init(obj);

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
