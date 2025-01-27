#define GLFW_INCLUDE_VULKAN
#include "stdio.h"
#include <GLFW/glfw3.h>

#include "clib/log.h"

void init_glfw(int gl_major, int gl_minor) {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, gl_major);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, gl_minor);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

void *create_glfw_window(int SCR_WIDTH, int SCR_HEIGHT,
                         char *null_terminated_name) {
  GLFWwindow *window =
      glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, null_terminated_name, NULL, NULL);
  if (window == NULL) {
    loge("Unable to create glfw window");
    glfwTerminate();
  }
  glfwMakeContextCurrent(window);
  return window;
}

int main() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan window", 0, 0);

  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(0, &extensionCount, 0);

  logp(" %i extensions supported\n", extensionCount);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window);

  glfwTerminate();

  return 0;
}
