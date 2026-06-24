#include <KHR\khrplatform.h>
#include <glad\glad.h>
#include <glfw3.h>
#include <glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/perpendicular.hpp>

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>

#include "..\shaders\Shader.h"
#include "Camera3.h"
#include "..\shaders\ComputeShader.h"



void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

const float TAU = 6.28318531;
const float PI = 3.141592654;
const float E = 2.7182818285;
const float G = 1.0;
float DT = 0.0001666;
const float C = 1.0;
const int numCubeVertices = 108;

float yaw = 0.0;
float pitch = 0.0;
bool firstMouse = true;
float lastX = SCR_WIDTH / 2;
float lastY = SCR_HEIGHT / 2;
Camera3D camera = Camera3D(glm::vec3(0.0f, 0.0f, 3.0f), 0.005f);


struct Vertex 
{
    glm::vec3 pos;
    glm::vec3 color;
};

struct Mesh
{
    GLuint VAO;
    GLuint VBO;
    GLsizei vertexCount;
};

struct instancedMesh
{
    GLuint VAO;
    GLuint VBO;
    GLuint instanceVBO;

    GLsizei vertexCount;
};

struct Mesh2
{
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;

    GLsizei vertexCount;
    GLsizei indexCount;
};

class FPSHandler
{
    public:
        float fps;
        float startTime;
        float endTime;
        int frames;

        FPSHandler();

        void fpsCheck()
        {
            if (endTime - startTime > 1.0)
            {
                fps = frames / (endTime - startTime);
                startTime = glfwGetTime();
                frames = 0;

                std::cout << "FPS: " << fps << "\r" << std::flush;
            }
        }
};

FPSHandler::FPSHandler()
{
    float fps = 0.0;
    float startTime = glfwGetTime();
    float endTime = 0.0;
    int frames = 0;
}

std::vector<Vertex> getCircleVert(float radius, glm::vec3 pos, glm::vec3 color, float da)
{
    std::vector<Vertex> vertices;

    vertices.push_back({
        pos,
        color
    });

    for (float i = 0.0; i < TAU; i += da)
    {
        float x = radius * cos(i);
        float y = radius * sin(i);

        vertices.push_back(
            {glm::vec3(x + pos.x, y + pos.y, 0.0f), color}
        );

        if (i + da > TAU)
        {
            vertices.push_back(
                {glm::vec3(radius + pos.x, 0.0f, 0.0f), color}
            );
        }
    }
    return vertices;
}

GLFWwindow* initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    return window;
}

Mesh2 bufferTriangle2(std::vector<Vertex> vertices, std::vector<unsigned int> indices, int buffer)
{
    Mesh2 mesh;
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    glGenBuffers(1, &mesh.EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    mesh.vertexCount = vertices.size();
    mesh.indexCount = indices.size();
    return mesh;
}

void updateUniforms(GLFWwindow* window, GLuint& windowSizeLoc)
{
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    glUniform2f(windowSizeLoc, width, height);
}

void updateUniformMatrices(Shader& ourShader, 
    glm::mat4& model, glm::mat4& view, glm::mat4& proj,
    GLuint& modelLoc, GLuint& viewLoc, GLuint& projLoc
)
{
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj));
}

// https://gamedev.stackexchange.com/questions/179426/c-generate-random-float-values-between-a-range
class RandomNumberGenerator
{
    std::random_device m_randomDevice{};
    std::mt19937 m_engine{m_randomDevice()};

    public:
        // Generates a random float in the range [low, high)
        float Generate(float low, float high)
        {
            return std::uniform_real_distribution<float>{low, high}(m_engine);
        }
};

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    camera.setCameraFront(glm::normalize(direction));
}

float lerp(float v0, float v1, float t) 
{
  return v0 + t * (v1 - v0);
}

Mesh bufferTriangle(std::vector<Vertex> vertices, int buffer)
{
    Mesh mesh;
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    mesh.vertexCount = vertices.size();
    return mesh;
}

std::vector<Vertex> vertToVectors(std::vector<float> vertices, glm::vec3 color)
{
    std::vector<Vertex> vectors;
    for (int i = 0; i < numCubeVertices; i+=3)
    {
        vectors.push_back({
            glm::vec3(vertices[i], vertices[i + 1], vertices[i + 2]),
            color
        });
    }

    return vectors;
}

std::vector<float> getCubeVert()
{
    std::vector<float> vec = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
        1.0f,  1.0f, -1.0f,
        1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f,  1.0f
    };
    return vec;
}

std::vector<float> getPrismVert(float height)
{
    // bottom of the prism is at 0.0
    std::vector<float> vec = {
        -1.0f,  height, -1.0f,
        -1.0f, 0.0f, -1.0f,
        1.0f, 0.0f, -1.0f,
        1.0f, 0.0f, -1.0f,
        1.0f,  height, -1.0f,
        -1.0f,  height, -1.0f,

        -1.0f, 0.0f,  1.0f,
        -1.0f, 0.0f, -1.0f,
        -1.0f,  height, -1.0f,
        -1.0f,  height, -1.0f,
        -1.0f,  height,  1.0f,
        -1.0f, 0.0f,  1.0f,

        1.0f, 0.0f, -1.0f,
        1.0f, 0.0f,  1.0f,
        1.0f,  height,  1.0f,
        1.0f,  height,  1.0f,
        1.0f,  height, -1.0f,
        1.0f, 0.0f, -1.0f,

        -1.0f, 0.0f,  1.0f,
        -1.0f,  height,  1.0f,
        1.0f,  height,  1.0f,
        1.0f,  height,  1.0f,
        1.0f, 0.0f,  1.0f,
        -1.0f, 0.0f,  1.0f,

        -1.0f,  height, -1.0f,
        1.0f,  height, -1.0f,
        1.0f,  height,  1.0f,
        1.0f,  height,  1.0f,
        -1.0f,  height,  1.0f,
        -1.0f,  height, -1.0f,

        -1.0f, 0.0f, -1.0f,
        -1.0f, 0.0f,  1.0f,
        1.0f, 0.0f, -1.0f,
        1.0f, 0.0f, -1.0f,
        -1.0f, 0.0f,  1.0f,
        1.0f, 0.0f,  1.0f
    };
    return vec;
}

std::vector<Vertex> getMapVert(int columns, int rows, glm::vec3 color)
{
    std::vector<Vertex> vertices;
    float invCols = 2.0 / columns;
    float invRows = 2.0 / rows;

    const float height = 0.3;
    const float phase = 5.0;

    for (float x = -1.0; x < 1.0; x += invCols)
    {
        for (float y = -1.0; y < 1.0; y += invRows)
        {
            float newX = x + invCols;
            float newY = y + invRows;

            vertices.push_back({glm::vec3(x, height * sin(phase * x * y), y), color});
            vertices.push_back({glm::vec3(x, height * sin(phase * x * newY), newY), color});
            vertices.push_back({glm::vec3(newX, height * sin(phase * newX * newY), newY), color});

            vertices.push_back({glm::vec3(newX, height * sin(phase * newX * newY), newY), color});
            vertices.push_back({glm::vec3(x, height * sin(phase * x * y), y), color});
            vertices.push_back({glm::vec3(newX, height * sin(phase * newX * y), y), color});
        }
    }

    return vertices;
}

std::vector<Vertex> multiplyVertices(std::vector<Vertex>& vertices0, float x)
{
    for (Vertex& v : vertices0)
        v.pos *= x;
    return vertices0;
}

int main(int argc, char** argv)
{
    GLFWwindow* window = initWindow();
    Shader mainShader("shaders/vshader.glsl", "shaders/fshader.glsl");
    GLuint windowSizeLoc = glGetUniformLocation(mainShader.ID,"windowSize");
    GLuint modelLoc = glGetUniformLocation(mainShader.ID, "model");
    GLuint viewLoc = glGetUniformLocation(mainShader.ID, "view");
    GLuint projLoc = glGetUniformLocation(mainShader.ID, "proj");

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    glEnable(GL_DEPTH_TEST);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);
    glm::mat4 proj = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    proj = glm::perspective(glm::radians(70.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));

    glm::vec2 windowSize = glm::vec2(SCR_WIDTH, SCR_HEIGHT);
    RandomNumberGenerator rng = RandomNumberGenerator();

    // setup FPS tracking
    FPSHandler fpsCounter = FPSHandler();

    // create scene objects
    // std::vector<Vertex> cubeVertices = vertToVectors(getPrismVert(5.0), glm::vec3(0.0f, 0.0, 0.5f));
    // Mesh cubeMesh = bufferTriangle(cubeVertices, 0);
    std::vector<Vertex> heightmapVertices = multiplyVertices(getMapVert(42, 42, glm::vec3(1.0, 0.0, 1.0)), 10.0);
    Mesh mapMesh = bufferTriangle(heightmapVertices, 0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    while (!glfwWindowShouldClose(window))
    {
        fpsCounter.frames += 1;
        fpsCounter.fpsCheck();
        // if (fpsCounter.startTime > 1.0f)
        //     DT = lerp(DT, 1.0 / fpsCounter.fps, 0.0001);

        glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mainShader.use();

        glBindBuffer(GL_ARRAY_BUFFER, mapMesh.VBO);
        glBindVertexArray(mapMesh.VAO);
        glDrawArrays(GL_TRIANGLES, 0, heightmapVertices.size());

        camera.doCameraMovement(window);
        camera.updateView(view);

        updateUniforms(window, windowSizeLoc);
        updateUniformMatrices(
            mainShader, model, view, proj, 
            modelLoc, viewLoc, projLoc
        );
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f + (fpsCounter.frames / 500.0)));

        glfwSwapBuffers(window);
        glfwPollEvents();

        fpsCounter.endTime = glfwGetTime();
    }

    return 0;
}