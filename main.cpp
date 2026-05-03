#include <iostream>
#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <Model.h>
#include <Shader.h>
#include <chrono>
#include <thread>

const float G = 0.1f;

struct PlanetData
{
    const char* name;
    float mass;
    float visualOrbitRadius;
    float modelScale;
};

struct PhysicsObject
{
    glm::vec3 position;
    glm::vec3 velocity;
    float     mass;
    Model*    model;
    const char* name;
    float     scale;
    std::vector<glm::vec3> trails;
    unsigned int trailVBO, trailVAO;
    glm::vec3 color;
};

const PlanetData Sun_Data     = { "Sun",     1000.0f,  0.0f,  0.009f  };
const PlanetData Earth_Data   = { "Earth",   0.04f,   122.0f, 0.3f };
const PlanetData Jupiter_Data = { "Jupiter", 0.954f,   200.0f, 0.05f };
const PlanetData Saturn_Data  = { "Saturn",  0.285f,   260.0f, 0.01f };
const PlanetData Mars_Data     = { "Mars",     0.0001f,  144.0f,  0.03f  };
const PlanetData Neptune_Data   = { "Neptune",   0.1f,   400.0f, 0.01f };
const PlanetData Uranus_Data = { "Uranus", 0.05f,   300.0f, 0.01f };
const PlanetData Moon_Data  = { "Moon",  0.000001f,   1.3f, 0.002f };
const PlanetData Mercury_Data = { "Mercury", 0.0003f,   65.0f, 0.01f };
const PlanetData Venus_Data  = { "Venus",  0.001f,   90.0f, 0.01f };

// Camera

glm::vec3 cameraPos   = glm::vec3(0.0f, 30.0f, 80.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, -0.3f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f,  1.0f,  0.0f);

int   WINDOW_WIDTH  = 1280;
int   WINDOW_HEIGHT = 720;
float deltaTime  = 0.0f;
float debug_timer = 0.0f;
float debug_interval = 1.0f;
float lastFrame  = 0.0f;
float yaw        = -90.0f;
float pitch      = -17.0f;
float lastX      = 640.0f;
float lastY      = 360.0f;
float cameraSpeed = 10.0f;
bool  firstMouse = true;

float simSpeed = 10.0f;
int   substeps = 50;


glm::vec3 computeAcc(int i, const std::vector<PhysicsObject>& objs) {
    glm::vec3 acc(0.0f);
    for (int j = 0; j < (int)objs.size(); j++) {
        if (i == j) continue;

        glm::vec3 r    = objs[j].position - objs[i].position;
        float dist2    = glm::dot(r, r) + 0.01f;  // softening
        float dist     = sqrtf(dist2);

        // a = G * Mj / |r|²  *  r_hat
        float forceMag = G * objs[j].mass / dist2;
        acc += (r / dist) * forceMag;
    }
    return acc;
}

// One-step Leapfrog KDK: Kick → Drift → Kick
void leapfrogStep(std::vector<PhysicsObject>& objs, float dt) {
    int n = (int)objs.size();

    // Step 1 calculate acceleration
    std::vector<glm::vec3> acc(n);
    for (int i = 0; i < n; i++)
        acc[i] = computeAcc(i, objs);

    // Step 2 v += a * dt/2
    for (int i = 0; i < n; i++)
        objs[i].velocity += acc[i] * (dt * 0.5f);

    // Step 3 x += v * dt
    for (int i = 0; i < n; i++)
        objs[i].position += objs[i].velocity * dt;

    // Step 4 Recalculate acceleration in new positions
    for (int i = 0; i < n; i++)
        acc[i] = computeAcc(i, objs);

    // Step 5 v += a_new * dt/2
    for (int i = 0; i < n; i++)
        objs[i].velocity += acc[i] * (dt * 0.5f);
}

//  v = sqrt(G*M_sun / r)
float circularOrbitSpeed(float orbitRadius) {
    return sqrtf((G * Sun_Data.mass) / orbitRadius);
}

// Callbacks

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    WINDOW_WIDTH  = width;
    WINDOW_HEIGHT = height;
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    float speed = 20.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;

    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET)  == GLFW_PRESS)
        simSpeed = glm::max(1.0f,      simSpeed * (1.0f - 2.0f * deltaTime));
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
        simSpeed = glm::min(100000.0f, simSpeed * (1.0f + 2.0f * deltaTime));
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cameraSpeed = 50000.0f;
    else
    {
        cameraSpeed = 5000.0f;
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos; lastY = (float)ypos;
        firstMouse = false;
    }
    float xoff = ((float)xpos - lastX) * 0.1f;
    float yoff = (lastY - (float)ypos) * 0.1f;
    lastX = (float)xpos; lastY = (float)ypos;
    yaw += xoff; pitch += yoff;
    pitch = glm::clamp(pitch, -89.0f, 89.0f);
    cameraFront = glm::normalize(glm::vec3(
        cosf(glm::radians(yaw)) * cosf(glm::radians(pitch)),
        sinf(glm::radians(pitch)),
        sinf(glm::radians(yaw)) * cosf(glm::radians(pitch))
    ));
}

int main() {
    std::cout << "  SOLAR SYSTEM START " << std::endl;

    if (!glfwInit()) { std::cerr << "Failed to initialize GLFW\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                          "Solar System", NULL, NULL);
    if (!window) { std::cerr << "Failed to create window\n"; glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n"; return -1;
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glEnable(GL_DEPTH_TEST);


    Shader ourShader("shaders/main.vert", "shaders/main.frag");
    Shader sunShader("shaders/sun.vert", "shaders/sun.frag");
    Shader traceShader("shaders/trace.vert", "shaders/trace.frag");

    Model Sun    ("models/Sun.fbx");
    Model Earth  ("models/Earth.fbx");
    Model Jupiter("models/Jupiter.fbx");
    Model Saturn ("models/Saturn.fbx");
    Model Mars ("models/Mars.fbx");
    Model Neptune ("models/Neptune.fbx");
    Model Uran ("models/Uranus.fbx");
    Model Venus ("models/Venus.fbx");
    Model Mercury ("models/Mercury.fbx");
    Model Moon ("models/Moon.fbx");

    std::vector<PhysicsObject> system;

    // Sun
    system.push_back({
        .position = glm::vec3(0.0f, 0.0f, 0.0f),  // Позиция: центр
        .velocity = glm::vec3(0.0f, 0.0f, 0.0f),  // Скорость: 0
        .mass = Sun_Data.mass,
        .model = &Sun,
        .name = "Sun",
        .scale = Sun_Data.modelScale
    });

    // Earth
    float v_earth = circularOrbitSpeed(Earth_Data.visualOrbitRadius);
    system.push_back({
        .position = glm::vec3(Earth_Data.visualOrbitRadius, 0.0f, 0.0f), // X = радиус, Z = 0
        .velocity = glm::vec3(0.0f, 0.0f, v_earth),                      // Скорость по Z
        .mass = Earth_Data.mass,
        .model = &Earth,
        .name = "Earth",
        .scale = Earth_Data.modelScale,
        .color = glm::vec3(1.0f, 0.0f, 0.0f)
    });

    // Moon
    float v_moon = sqrtf(G * Earth_Data.mass / Moon_Data.visualOrbitRadius) * 1.15f;
    float v_y = v_moon * sin(glm::radians(30.0f));
    float v_z = v_moon * cos(glm::radians(30.0f));
    system.push_back({
        .position = system[1].position + glm::vec3(Moon_Data.visualOrbitRadius, 0.0f, 0.0f),
        .velocity = system[1].velocity +  glm::vec3(0.0f, v_y, v_z),
        .mass = Moon_Data.mass,
        .model = &Moon,
        .name = "Moon",
        .scale = Moon_Data.modelScale,
        .color = glm::vec3(1.0f, 1.0f, 1.0f)
    });

    // Jupiter
    float v_jupiter = circularOrbitSpeed(Jupiter_Data.visualOrbitRadius);
    system.push_back({
        .position = glm::vec3(0.0f, 0.0f, Jupiter_Data.visualOrbitRadius), // X = 0, Z = радиус
        .velocity = glm::vec3(-v_jupiter, 0.0f, 0.0f),                     // Скорость по -X
        .mass = Jupiter_Data.mass,
        .model = &Jupiter,
        .name = "Jupiter",
        .scale = Jupiter_Data.modelScale,
        .color = glm::vec3(1.0f, 1.0f, 1.0f)
    });

    // Saturn
    float v_saturn = circularOrbitSpeed(Saturn_Data.visualOrbitRadius);
    system.push_back({
        .position = glm::vec3(-Saturn_Data.visualOrbitRadius, 0.0f, 0.0f), // X = -радиус, Z = 0
        .velocity = glm::vec3(0.0f, 0.0f, -v_saturn),                      // Скорость по -Z
        .mass = Saturn_Data.mass,
        .model = &Saturn,
        .name = "Saturn",
        .scale = Saturn_Data.modelScale,
        .color = glm::vec3(0.0f, 1.0f, 0.0f)
    });

    // Mars
    float v_mars = circularOrbitSpeed(Mars_Data.visualOrbitRadius);
    system.push_back({
        .position = glm::vec3(Mars_Data.visualOrbitRadius, 0.0f, 0.0f), // X = -радиус, Z = 0
        .velocity = glm::vec3(0.0f, 0.0f, -v_mars),                      // Скорость по -Z
        .mass = Mars_Data.mass,
        .model = &Mars,
        .name = "Mars",
        .scale = Mars_Data.modelScale,
        .color = glm::vec3(0.0f, 1.0f, 1.0f)
    });

    // Neptune
    float v_neptune = circularOrbitSpeed(Neptune_Data.visualOrbitRadius);
    system.push_back({
        .position = glm::vec3(Neptune_Data.visualOrbitRadius, 0.0f, 0.0f), // X = -радиус, Z = 0
        .velocity = glm::vec3(0.0f, 0.0f, -v_neptune),                      // Скорость по -Z
        .mass = Neptune_Data.mass,
        .model = &Neptune,
        .name = "Neptune",
        .scale = Neptune_Data.modelScale,
        .color = glm::vec3(0.8f, 0.4f, 0.8f)
    });

    // Uranus
    float v_uranus = circularOrbitSpeed(Uranus_Data.visualOrbitRadius);
    system.push_back({
        .position = glm::vec3(Uranus_Data.visualOrbitRadius, 0.0f, 0.0f), // X = -радиус, Z = 0
        .velocity = glm::vec3(0.0f, 0.0f, -v_uranus),                      // Скорость по -Z
        .mass = Uranus_Data.mass,
        .model = &Uran,
        .name = "Uranus",
        .scale = Uranus_Data.modelScale,
        .color = glm::vec3(0.4f, 1.0f, 0.2f)
    });

    // Venus
    float v_venus = circularOrbitSpeed(Venus_Data.visualOrbitRadius);
    system.push_back({
        .position = glm::vec3(Venus_Data.visualOrbitRadius, 0.0f, 0.0f), // X = -радиус, Z = 0
        .velocity = glm::vec3(0.0f, 0.0f, -v_venus),                      // Скорость по -Z
        .mass = Venus_Data.mass,
        .model = &Venus,
        .name = "Venus",
        .scale = Venus_Data.modelScale,
        .color = glm::vec3(0.4f, 1.0f, 0.2f)
    });

    // Mercury
    float v_mercury = circularOrbitSpeed(Mercury_Data.visualOrbitRadius);
    system.push_back({
        .position = glm::vec3(-Mercury_Data.visualOrbitRadius, 0.0f, 0.0f), // X = -радиус, Z = 0
        .velocity = glm::vec3(0.0f, 0.0f, -v_mercury),                      // Скорость по -Z
        .mass = Mercury_Data.mass,
        .model = &Mercury,
        .name = "Mercury",
        .scale = Mercury_Data.modelScale,
        .color = glm::vec3(0.4f, 1.0f, 0.2f)
    });


    for (auto &obj : system)
    {
        glGenVertexArrays(1, &obj.trailVAO);
        glGenBuffers(1, &obj.trailVBO);

        glBindVertexArray(obj.trailVAO);
        glBindBuffer(GL_ARRAY_BUFFER, obj.trailVBO);

        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 5000, nullptr, GL_DYNAMIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glEnableVertexAttribArray(0);
    }

    glm::vec3 totalMomentum(0.0f);
    for (size_t i = 1; i < system.size(); i++) {
        totalMomentum += system[i].velocity * system[i].mass;
    }

    // Give to the Sun inverse impulse
    system[0].velocity = -totalMomentum / system[0].mass;

    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        deltaTime = currentTime - lastFrame;
        lastFrame = currentTime;

        if (deltaTime > 0.1f) deltaTime = 0.016f;

        processInput(window);

        glClearColor(0.02f, 0.02f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
            0.1f, 1000000.0f
        );
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view",       view);
        ourShader.setVec3("lightColor", glm::vec3(1.0f, 0.9f, 0.8f));
        ourShader.setVec3("viewPos",    cameraPos);

        substeps = glm::clamp((int)(simSpeed / 10.0f) + 50, 100, 5000);

        float dt_total = deltaTime * simSpeed / (float)substeps;
        for (int s = 0; s < substeps; s++)
            leapfrogStep(system, dt_total);


        for (auto  &obj : system)
        {
            obj.trails.push_back(obj.position);

             if (obj.trails.size() > 3500)
             {
                 obj.trails.erase(obj.trails.begin());
            }

            glBindVertexArray(obj.trailVAO);
            glBindBuffer(GL_ARRAY_BUFFER, obj.trailVBO);
            glBufferData(GL_ARRAY_BUFFER, obj.trails.size() * sizeof(glm::vec3), &obj.trails[0], GL_DYNAMIC_DRAW);
        }
        traceShader.use();
        traceShader.setMat4("projection", projection);
        traceShader.setMat4("view", view);
        traceShader.setMat4("model", glm::mat4(1.0f));

        for (auto  &obj : system)
        {
            traceShader.setVec3("lineColor", obj.color);
            glBindVertexArray(obj.trailVAO);
            glDrawArrays(GL_LINE_STRIP, 0, obj.trails.size());
        }

        debug_timer += deltaTime;
        // Render
        ourShader.use();
        for (int i = 0; i < (int)system.size(); i++) {
            glm::mat4 m = glm::mat4(1.0f);
            m = glm::translate(m, system[i].position);
            m = glm::scale(m, glm::vec3(system[i].scale));

            if (i == 0)
            {
                ourShader.setVec3("lightPos", system[0].position);

                sunShader.use();
                sunShader.setMat4("projection", projection);
                sunShader.setMat4("view", view);
                sunShader.setMat4("model", m);
                system[0].model->Draw(sunShader);

            }
            else
            {
                ourShader.use();
                ourShader.setMat4("model", m);
                system[i].model->Draw(ourShader);
            }

        }

        if (debug_timer >= debug_interval)
        {
            for (int i = 0; i < (int)system.size(); i++)
            {
                std::cout << system[i].name << " " << system[i].position.x << " | " << system[i].position.y << " | " << system[i].position.z << std::endl;

            }
            debug_timer -= debug_interval;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}