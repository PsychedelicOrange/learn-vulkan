#include <string.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "clib/log.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <stdint.h>

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

static VkBool32
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageTypes,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  (void)pUserData;
  (void)messageTypes;
  VkDebugUtilsMessengerCallbackDataEXT *dto =
      (VkDebugUtilsMessengerCallbackDataEXT *)pCallbackData;
  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    logi("[ValidationLayer]: %s", dto->pMessage);
  };
  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    logw("[ValidationLayer]: %s", dto->pMessage);
  }
  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    loge("[ValidationLayer]: %s", dto->pMessage);
  }
  return VK_FALSE;
}

VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance) {
  VkDebugUtilsMessengerCreateInfoEXT createInfo = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = debugCallback,
      .pUserData = NULL,
  };
  VkDebugUtilsMessengerEXT messenger = NULL;
  PFN_vkCreateDebugUtilsMessengerEXT function =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkCreateDebugUtilsMessengerEXT");

  if (function != NULL) {
    if (function(instance, &createInfo, NULL, &messenger) != VK_SUCCESS) {
      loge("Couldn't set up debug messenger");
    }
  } else {
    loge("debug messenger extension not present");
  }
  return messenger;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
  PFN_vkDestroyDebugUtilsMessengerEXT function =
      (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
          instance, "vkDestroyDebugUtilsMessengerEXT");
  if (function != NULL) {
    function(instance, debugMessenger, pAllocator);
  }
}

VkInstance createInstance() {

  // check instance extension support
  {
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(0, &extensionCount, 0);
    logi(" %i Instance extensions supported", extensionCount);
    VkExtensionProperties extensionProperties[extensionCount];
    vkEnumerateInstanceExtensionProperties(0, &extensionCount,
                                           extensionProperties);
    for (uint32_t i = 0; i < extensionCount; i++) {
      logi("%s,", extensionProperties[i].extensionName);
    }
  }

  // check instance validation layer support
  const char *layersEnable[1] = {"VK_LAYER_KHRONOS_validation"};
  {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    logi("%i Available Instance layers", layerCount);
    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
    int is_valdation_available = 0;
    for (uint32_t i = 0; i < layerCount; i++) {
      logi("%s,", availableLayers[i].layerName);
      if (strcmp(layersEnable[0], availableLayers[i].layerName) == 0) {
        is_valdation_available = 1;
      }
    }

    if (is_valdation_available) {
      logi("Vulkan validation layer is available");
    } else {
      loge("Vulkan validation layer is not available");
      exit(1);
    }
  }

  VkApplicationInfo applicationInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "learn-vulkan",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "none",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_MAKE_VERSION(1, 4, 304)};

#ifdef DISABLE_VALIDATION_LAYER
  int enableValidationLayers = 0;
#else
  int enableValidationLayers = 1;
#endif

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  const char *extensions[glfwExtensionCount + 1];
  for (uint32_t i = 0; i < glfwExtensionCount; i++)
    extensions[i] = glfwExtensions[i];
  if (enableValidationLayers) {
    extensions[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
  }

  VkInstanceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &applicationInfo,
      .enabledExtensionCount = glfwExtensionCount,
      .ppEnabledExtensionNames = extensions};
  if (enableValidationLayers) {
    createInfo.enabledLayerCount = 1;
    createInfo.enabledExtensionCount = glfwExtensionCount + 1;
    createInfo.ppEnabledLayerNames = layersEnable;
  } else {
    createInfo.enabledLayerCount = 0;
  }

  VkInstance instance;
  VkResult result;
  if ((result = vkCreateInstance(&createInfo, NULL, &instance)) != VK_SUCCESS) {
    loge("VkResult[%i]: Couldn't create vulkan instance!", result);
  }
  return instance;
}

int main() {
  init_glfw();
  GLFWwindow *window = create_glfw_window(800, 600, "Vulkan window");

  VkInstance instance = createInstance();
  VkDebugUtilsMessengerEXT debugMessenger = createDebugMessenger(instance);

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  DestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
  vkDestroyInstance(instance, NULL);
  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
