#include <glad/glad.h>
#include <glfw3.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include "struct.h"
#include "file_util.h"
#include "obj_loader.h"

#ifndef M_PI
#define M_PI 3.1415926
#endif

float radians(float deg) {return deg * (M_PI / 180.0f);}

#define WIDTH 1280
#define HEIGHT 720

bool g_firstMouse = true;
float g_lastX = WIDTH / 2.0f;
float g_lastY = HEIGHT / 2.0f;
float g_mouseSensitivity = 0.1f;

int g_frameCount = 0;
Camera g_camera = {0.0f, 0.0f, 2.0f, -90.0f, 0.0f, 1.0f};
float g_cameraSpeed = 2.5f;

float g_lastFrame = 0.0f;
float g_deltaTime = 0.0f;

bool g_framebufferResized = false;
int g_newWidth = WIDTH;
int g_newHeight = HEIGHT;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    g_newWidth = width;
    g_newHeight = height;
    g_framebufferResized = true;
}

void calculateCameraVectors(Camera* camera, float* forwardX, float* forwardZ, float* rightX, float* rightZ)
{
    float yawRads = radians(camera->yaw);

    *forwardX = cos(yawRads);
    *forwardZ = sin(yawRads);

    *rightX = cos(yawRads + radians(90.0f));
    *rightZ = sin(yawRads + radians(90.0f));
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (g_firstMouse)
    {
        g_lastX = (float)xpos;
        g_lastY = (float)ypos;
        g_firstMouse = false;
        return;
    }

    float xoffset = (float)xpos - g_lastX;
    float yoffset = g_lastY - (float)ypos;
    g_lastX = (float)xpos;
    g_lastY = (float)ypos;

    xoffset *= g_mouseSensitivity;
    yoffset *= g_mouseSensitivity;

    g_camera.yaw += xoffset;
    g_camera.pitch += yoffset;

    if (g_camera.pitch > 89.0f) {g_camera.pitch = 89.0f;}
    if (g_camera.pitch < -89.0f) {g_camera.pitch = -89.0f;}

    g_frameCount = 0;
}

bool processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    bool moved = false;
    float camVelocity = g_cameraSpeed * g_deltaTime;

    float forwardX, forwardZ, rightX, rightZ;
    calculateCameraVectors(&g_camera, &forwardX, &forwardZ, &rightX, &rightZ);

    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        g_camera.px += forwardX * camVelocity;
        g_camera.pz += forwardZ * camVelocity;
        moved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        g_camera.px -= forwardX * camVelocity;
        g_camera.pz -= forwardZ * camVelocity;
        moved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        g_camera.px -= rightX * camVelocity;
        g_camera.pz -= rightZ * camVelocity;
        moved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        g_camera.px += rightX * camVelocity;
        g_camera.pz += rightZ * camVelocity;
        moved = true;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        g_camera.py += camVelocity;
        moved = true;
    }
    
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    {
        g_camera.py -= camVelocity;
        moved = true;
    }

    return moved;
}

GLuint CompileShader(const char* filename, GLenum type)
{

    char* source = ReadFileToString(filename);
    if (source == NULL) {return 0;}

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, (const char* const*)&source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        fprintf(stderr, "Shader compile error in &s: \n%s\n", filename, infoLog);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint CreateShaderProgram()
{
    GLuint vertexShader = CompileShader("shaders/fullscreen.vert", GL_VERTEX_SHADER);
    GLuint fragmentShader = CompileShader("shaders/raytrace.frag", GL_FRAGMENT_SHADER);

    if (vertexShader == 0 || fragmentShader == 0) {return 0;}

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Linking error \n%s\n", infoLog);
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// FBO
GLuint g_fbo;
GLuint g_accumTexture;
GLuint g_outputTexture;

// Frame Buffer init
void SetupAccumulationBuffers(int width, int height)
{
    glGenFramebuffers(1, &g_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);

    // TX1: Accumulated
    glGenTextures(1, &g_accumTexture);
    glBindTexture(GL_TEXTURE_2D, g_accumTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // TX2: Output
    glGenTextures(1, &g_outputTexture);
    glBindTexture(GL_TEXTURE_2D, g_outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_accumTexture, 0);

    GLenum drawBuffers[] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(1, drawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Framebuffer incomplete\n");
    }

    // Unbind FBO
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Material g_materials[] =
{
    // Matte Green 0
    {0.2f, 1.0f, 0.2f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f}, 

    // Mirror 1
    {1.0f, 1.0f, 1.0f, 0.0f, 0.2f, 1.0f, 0.0f, 1.0f}, 

    // Emissive White 2
    {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 25.0f, 1.0f},

    {0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 15.0f, 1.0f},

    // Semi-Transparrent Blue 4
    {0.2f, 0.2f, 1.0f, 0.0f, 0.2f, 0.0f, 0.0f, 0.2f}, 
 
    // Matte white 5
    {1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f} 
};
const int NUM_MATERIALS = sizeof(g_materials) / sizeof(Material);

void SetupSceneData(GLuint sphere_ssbo, GLuint material_ssbo, GLuint vertex_ssbo, GLuint index_ssbo)
{
    // Load OBJ
    MeshData mesh = load_obj("a.obj");

    if (mesh.vertices != NULL && mesh.indices != NULL)
    {
        fprintf(stderr, "Loaded mesh with %zu v, %zu i\n", mesh.num_vertices / 3, mesh.num_indices);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertex_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, mesh.num_vertices * sizeof(float), mesh.vertices, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, vertex_ssbo);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, index_ssbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, mesh.num_indices * sizeof(unsigned int), mesh.indices, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, index_ssbo);

        free_mesh_data(&mesh);
    }   
    else {fprintf(stderr, "Failed to load OBJ");}

    // Setup spheres
    Sphere scene[5];

    //scene[0] = (Sphere){0.0f, 0.0f, 0.0f, 1.0f, 1};
    //scene[1] = (Sphere){2.5f, 0.0f, 0.0f, 0.5f, 3};
    //scene[2] = (Sphere){-2.5f, 0.0f, 0.0f, 0.5f, 2}; 
    //scene[3] = (Sphere){0.0f, 0.0f, -3.0f, 1.0f, 5}; 
    //scene[4] = (Sphere){0.0f, 0.0f, 3.0f, 1.0f, 5}; 

    // Sphere Data
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sphere_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(scene), scene, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, sphere_ssbo); 

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, material_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(g_materials), g_materials, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, material_ssbo);
}

int main(int argc, char* argv[])
{
    // GLFW Init
    if (!glfwInit())
    {
        printf("GLFW init failed\n");
        return 1;
    }

    // OpenGL Attributes
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    // Create Window
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "RTX", NULL, NULL);
    if (!window)
    {
        fprintf(stderr, "GLFW window creation failed\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Capture And Hide Mouse Pointer
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    // Load OpenGL Functions 
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "Failed to load GLAD\n");
        glfwTerminate();
        return 1;
    }

    // Disable vSync
    glfwSwapInterval(0);

    // Fix Black Screen At Startup
    int initialWidth, initialheight;
    glfwGetFramebufferSize(window, &initialWidth, &initialheight);

    framebuffer_size_callback(window, initialWidth, initialheight);

    // Buffer Setup
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint ssbo_spheres;
    GLuint ssbo_materials;
    GLuint ssbo_vertices;
    GLuint ssbo_indices;

    glGenBuffers(1, &ssbo_spheres);
    glGenBuffers(1, &ssbo_materials);
    glGenBuffers(1, &ssbo_vertices);
    glGenBuffers(1, &ssbo_indices);

    SetupSceneData(ssbo_spheres, ssbo_materials, ssbo_vertices, ssbo_indices);

    GLuint program = CreateShaderProgram();
    glUseProgram(program);

    //glUniform2f(glGetUniformLocation(program, "u_resolution"), (float)WIDTH, (float)HEIGHT);

    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        g_deltaTime = currentFrame - g_lastFrame;
        g_lastFrame = currentFrame;
            
        // Handle Window Resize
        if (g_framebufferResized)
        {
            glDeleteTextures(1, &g_accumTexture);
            glDeleteTextures(1, &g_outputTexture);
            glDeleteFramebuffers(1, &g_fbo);

            SetupAccumulationBuffers(g_newWidth, g_newHeight);

            g_frameCount = 0;
            g_framebufferResized = false;
        }

        bool cameraMoved = processInput(window);
        if (cameraMoved) {g_frameCount = 0;}
        g_frameCount++;

        glUniform2f(glGetUniformLocation(program, "u_resolution"), (float)g_newWidth, (float)g_newHeight);

        glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_accumTexture, 0);

        glUniform1i(glGetUniformLocation(program, "u_frameCount"), g_frameCount);
        glUniform1i(glGetUniformLocation(program, "u_historyTexture"), 0);
        glUniform3f(glGetUniformLocation(program, "u_cameraPos"), g_camera.px, g_camera.py, g_camera.pz);
        glUniform1f(glGetUniformLocation(program, "u_cameraYaw"), g_camera.yaw);
        glUniform1f(glGetUniformLocation(program, "u_cameraPitch"), g_camera.pitch);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_outputTexture);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        GLuint temp = g_accumTexture;
        g_accumTexture = g_outputTexture;
        g_outputTexture = temp;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, g_accumTexture);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}