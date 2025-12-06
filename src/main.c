#include <glad/glad.h>
#include <glfw3.h>
#include <stdio.h>
#include <stdbool.h>

#include "struct.h"
#include "file_util.h"

#define WIDTH 1280
#define HEIGHT 720

int g_frameCount = 0;
Camera g_camera = {0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 1.0f};
float g_cameraSpeed = 0.05f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

bool processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    bool moved = false;
    float delta = g_cameraSpeed;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        g_camera.pz -= delta; moved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        g_camera.pz += delta; moved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        g_camera.px -= delta; moved = true;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        g_camera.px += delta; moved = true;
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
void SetupAccumulationBuffers()
{
    glGenFramebuffers(1, &g_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);

    // TX1: Accumulated
    glGenTextures(1, &g_accumTexture);
    glBindTexture(GL_TEXTURE_2D, g_accumTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // TX2: Output
    glGenTextures(1, &g_outputTexture);
    glBindTexture(GL_TEXTURE_2D, g_outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
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

void SetupSceneData(GLuint ssbo)
{
    Sphere scene[3];

    scene[0] = (Sphere){0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.2f, 0.2f, 0.0f};
    scene[1] = (Sphere){0.0f, -101.0f, 0.0f, 100.0f, 0.2f, 1.0f, 0.2f, 0.0f}; 
    scene[2] = (Sphere){1.5f, 0.0f, 0.0f, 0.5f, 0.2f, 0.2f, 1.0f, 0.0f};

    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(scene), scene, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo); // Bind to binding = 0
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

    // Load OpenGL Functions 
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        fprintf(stderr, "Failed to load GLAD\n");
        glfwTerminate();
        return 1;
    }

    // Disable VSync
    glfwSwapInterval(0);

    // Buffer Setup
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    SetupSceneData(ssbo);

    GLuint program = CreateShaderProgram();
    glUseProgram(program);

    glUniform2f(glGetUniformLocation(program, "u_resolution"), (float)WIDTH, (float)HEIGHT);


    SetupAccumulationBuffers();

    // Main Loop
    while (!glfwWindowShouldClose(window))
    {
        bool cameraMoved = processInput(window);
        if (cameraMoved) {g_frameCount = 0;}
        g_frameCount++;

        glBindFramebuffer(GL_FRAMEBUFFER, g_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, g_accumTexture, 0);

        glUniform1i(glGetUniformLocation(program, "u_frameCount"), g_frameCount);
        glUniform1i(glGetUniformLocation(program, "u_historyTexture"), 0);
        glUniform3f(glGetUniformLocation(program, "u_cameraPos"), g_camera.px, g_camera.py, g_camera.pz);

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