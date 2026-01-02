
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "objects.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <iostream>
#include <vector>

using namespace glm;

// ---------------- Shader helpers ----------------
static GLuint compileShader(GLenum type, const char *src) {
  GLuint s = glCreateShader(type);
  glShaderSource(s, 1, &src, nullptr);
  glCompileShader(s);
  GLint ok;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[1024];
    glGetShaderInfoLog(s, 1024, nullptr, log);
    std::cerr << "Shader error:\n" << log << "\n";
  }
  return s;
}

static GLuint makeProgram(const char *vs, const char *fs) {
  GLuint v = compileShader(GL_VERTEX_SHADER, vs);
  GLuint f = compileShader(GL_FRAGMENT_SHADER, fs);
  GLuint p = glCreateProgram();
  glAttachShader(p, v);
  glAttachShader(p, f);
  glLinkProgram(p);
  glDeleteShader(v);
  glDeleteShader(f);
  return p;
}

// ---------------- Sphere mesh ----------------
static GLuint sphereVAO = 0, sphereVBO = 0, sphereEBO = 0;
static GLsizei indexCount = 0;

static void buildSphere(int stacks, int slices) {
  std::vector<float> verts;
  std::vector<unsigned int> indices;

  for (int i = 0; i <= stacks; i++) {
    float v = (float)i / stacks;
    float phi = v * pi<float>();

    for (int j = 0; j <= slices; j++) {
      float u = (float)j / slices;
      float theta = u * two_pi<float>();

      float x = sin(phi) * cos(theta);
      float y = cos(phi);
      float z = sin(phi) * sin(theta);

      // position
      verts.push_back(x);
      verts.push_back(y);
      verts.push_back(z);
      // normal
      verts.push_back(x);
      verts.push_back(y);
      verts.push_back(z);
    }
  }

  for (int i = 0; i < stacks; i++) {
    for (int j = 0; j < slices; j++) {
      int a = i * (slices + 1) + j;
      int b = a + slices + 1;

      indices.push_back(a);
      indices.push_back(b);
      indices.push_back(a + 1);

      indices.push_back(b);
      indices.push_back(b + 1);
      indices.push_back(a + 1);
    }
  }

  indexCount = (GLsizei)indices.size();

  glGenVertexArrays(1, &sphereVAO);
  glGenBuffers(1, &sphereVBO);
  glGenBuffers(1, &sphereEBO);

  glBindVertexArray(sphereVAO);

  glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
  glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(),
               GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               indices.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                        (void *)(3 * sizeof(float)));
  glEnableVertexAttribArray(1);

  glBindVertexArray(0);
}

// ---------------- BlackHole::draw ----------------
static GLuint program;
static mat4 projection, view;
static vec3 cameraPos = vec3(0.0f, 0.0f, 5.0f);
static vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
static vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
static vec3 target = vec3(0.0f);
static float distanceToTarget = 5.0f;
static float yawDeg = -90.0f;
static float pitchDeg = 15.0f;

static bool dragging = false;
static double lastX = 0.0, lastY = 0.0;
static float sensitivity = 0.2f;

static float lastTime = 0.0f;
static float moveSpeed = 3.0f;

static void processMovement(GLFWwindow *window, float dt) {
  float v = moveSpeed * dt;

  // forward/back
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    cameraPos += cameraFront * v;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    cameraPos -= cameraFront * v;

  // strafe
  glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    cameraPos += right * v;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    cameraPos -= right * v;

  // optional: up/down (space/shift)
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    cameraPos += cameraUp * v;
  if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    cameraPos -= cameraUp * v;
}

static void mouse_button_callback(GLFWwindow *window, int button, int action,
                                  int mods) {
  (void)mods;
  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      dragging = true;
      glfwGetCursorPos(window, &lastX, &lastY);
    } else if (action == GLFW_RELEASE) {
      dragging = false;
    }
  }
}

static void cursor_position_callback(GLFWwindow *window, double xpos,
                                     double ypos) {
  (void)window;
  if (!dragging)
    return;

  double dx = xpos - lastX;
  double dy = ypos - lastY;
  lastX = xpos;
  lastY = ypos;

  yawDeg += (float)dx * sensitivity;
  pitchDeg -=
      (float)dy * sensitivity; // invert so dragging up looks down at target

  // clamp pitch to avoid flipping
  if (pitchDeg > 89.0f)
    pitchDeg = 89.0f;
  if (pitchDeg < -89.0f)
    pitchDeg = -89.0f;
}

static glm::mat4 computeOrbitView() {
  float yaw = glm::radians(yawDeg);
  float pitch = glm::radians(pitchDeg);

  glm::vec3 dir;
  dir.x = cosf(yaw) * cosf(pitch);
  dir.y = sinf(pitch);
  dir.z = sinf(yaw) * cosf(pitch);
  dir = glm::normalize(dir);

  glm::vec3 cameraPos = target - dir * distanceToTarget;
  return glm::lookAt(cameraPos, target, glm::vec3(0.0f, 1.0f, 0.0f));
}

void BlackHole::draw() {
  mat4 model = translate(mat4(1.0f), vec3(position.x, position.y, position.z));
  model = scale(model, vec3((float)r_s * 1e-4f)); // scale to visible size

  mat4 MVP = projection * view * model;

  glUseProgram(program);
  glUniformMatrix4fv(glGetUniformLocation(program, "uMVP"), 1, GL_FALSE,
                     value_ptr(MVP));
  glUniformMatrix4fv(glGetUniformLocation(program, "uModel"), 1, GL_FALSE,
                     value_ptr(model));
  glUniform3f(glGetUniformLocation(program, "uLightDir"), -0.5f, -1.0f, -0.3f);

  glBindVertexArray(sphereVAO);
  glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

// ---------------- Main ----------------
int main() {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  GLFWwindow *window =
      glfwCreateWindow(800, 600, "Black Hole (Sphere)", nullptr, nullptr);
  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);
  glEnable(GL_DEPTH_TEST);

  const char *vs = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;

    uniform mat4 uMVP;
    uniform mat4 uModel;

    out vec3 Normal;
    out vec3 FragPos;

    void main() {
      FragPos = vec3(uModel * vec4(aPos, 1.0));
      Normal = mat3(transpose(inverse(uModel))) * aNormal;
      gl_Position = uMVP * vec4(aPos, 1.0);
    }
  )";

  const char *fs = R"(
    #version 330 core
    in vec3 Normal;
    in vec3 FragPos;
    out vec4 FragColor;

    uniform vec3 uLightDir;

    void main() {
      vec3 n = normalize(Normal);
      float diff = max(dot(n, normalize(-uLightDir)), 0.0);
      vec3 color = vec3(0.05) + diff * vec3(0.6);
      FragColor = vec4(color, 1.0);
    }
  )";

  program = makeProgram(vs, fs);
  buildSphere(64, 64);

  projection = perspective(radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);
  view = lookAt(vec3(0, 0, 5), vec3(0), vec3(0, 1, 0));

  BlackHole bh({0.0, 0.0, 0.0}, 5.0e30);

  while (!glfwWindowShouldClose(window)) {
    float now = (float)glfwGetTime();
    float dt = now - lastTime;
    lastTime = now;

    processMovement(window, dt);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);
    view = lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    view = computeOrbitView();
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bh.draw();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
