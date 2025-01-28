#include <stdint.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include "stdio.h"
#include <GLFW/glfw3.h>

#include "clib/log.h"

void init_glfw() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
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

void createInstance() {

  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(0, &extensionCount, 0);
  logi(" %i extensions supported", extensionCount);
  VkExtensionProperties extensionProperties[extensionCount];
  vkEnumerateInstanceExtensionProperties(0, &extensionCount,
                                         extensionProperties);
  for (uint32_t i = 0; i < extensionCount; i++) {
    logi("%s,", extensionProperties[i].extensionName);
  }

  VkApplicationInfo applicationInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "learn-vulkan",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "none",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_MAKE_VERSION(1, 4, 304)};

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  VkInstanceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &applicationInfo,
      .enabledLayerCount = 0,
      .enabledExtensionCount = glfwExtensionCount,
      .ppEnabledExtensionNames = glfwExtensions};

  VkInstance instance;
  VkResult result;
  if ((result = vkCreateInstance(&createInfo, NULL, &instance)) != VK_SUCCESS) {
    loge("VkResult[%i]: Couldn't create vulkan instance!", result);
  }
}

int main() {
  init_glfw();
  GLFWwindow *window = create_glfw_window(800, 600, "Vulkan window");

  createInstance();

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
