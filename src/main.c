#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "stdio.h"

int main() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan window", 0, 0);

  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(0, &extensionCount, 0);

  printf(" %i extensions supported\n", extensionCount);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window);

  glfwTerminate();

  return 0;
}
