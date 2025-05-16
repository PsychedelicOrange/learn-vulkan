#include <string.h>
#include <vulkan/vulkan_core.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "clib/log.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <stdint.h>

typedef struct
{
    uint32_t *ptr;
    size_t size;
} code;

struct cleanup
{
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkFramebuffer *swapChainFramebuffers;
} global;

code read_shader(const char *filename)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        loge("Could not open file %s\n", filename);
        fflush(stdout);
        return (code){.size = 0, .ptr = NULL};
    }

    // Move the file pointer to the end of the file to determine file size
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file); // Move file pointer back to the beginning

    if (fileSize % sizeof(uint32_t) != 0)
    {
        loge("Invalid SPIR-V file size");
        fclose(file);
        return (code){.size = 0, .ptr = NULL};
    }

    // Allocate memory for the file content
    uint32_t *content = (uint32_t *)malloc(fileSize);
    if (!content)
    {
        loge("Memory allocation failed\n");
        fclose(file);
        return (code){.size = 0, .ptr = NULL};
    }
    size_t read_count = fread(content, 1, fileSize, file);
    // Close the file and return the content
    fclose(file);
    if (read_count != fileSize)
    {
        loge("Failed to read full shader file");
        exit(1);
    }

    return (code){.ptr = (uint32_t *)content, .size = fileSize};
}

void init_glfw()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

void *create_glfw_window(int SCR_WIDTH, int SCR_HEIGHT, char *null_terminated_name)
{
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, null_terminated_name, NULL, NULL);
    if (window == NULL)
    {
        loge("Unable to create glfw window");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);
    return window;
}

static VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                              VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData)
{
    (void)pUserData;
    (void)messageTypes;
    VkDebugUtilsMessengerCallbackDataEXT *dto = (VkDebugUtilsMessengerCallbackDataEXT *)pCallbackData;
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        logi("[ValidationLayer]: %s", dto->pMessage);
    };
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        logw("[ValidationLayer]: %s", dto->pMessage);
    }
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        loge("[ValidationLayer]: %s", dto->pMessage);
    }
    return VK_FALSE;
}

VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance)
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData = NULL,
    };
    VkDebugUtilsMessengerEXT messenger = NULL;
    PFN_vkCreateDebugUtilsMessengerEXT function =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (function != NULL)
    {
        if (function(instance, &createInfo, NULL, &messenger) != VK_SUCCESS)
        {
            loge("Couldn't set up debug messenger");
        }
    }
    else
    {
        loge("debug messenger extension not present");
    }
    return messenger;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator)
{
    PFN_vkDestroyDebugUtilsMessengerEXT function =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (function != NULL)
    {
        function(instance, debugMessenger, pAllocator);
    }
}

VkInstance createInstance()
{

    // check instance extension support
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(0, &extensionCount, 0);
        logi(" %i Instance extensions supported", extensionCount);
        VkExtensionProperties extensionProperties[extensionCount];
        vkEnumerateInstanceExtensionProperties(0, &extensionCount, extensionProperties);
        for (uint32_t i = 0; i < extensionCount; i++)
        {
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
        for (uint32_t i = 0; i < layerCount; i++)
        {
            logi("%s,", availableLayers[i].layerName);
            if (strcmp(layersEnable[0], availableLayers[i].layerName) == 0)
            {
                is_valdation_available = 1;
            }
        }

        if (is_valdation_available)
        {
            logi("Vulkan validation layer is available");
        }
        else
        {
            loge("Vulkan validation layer is not available");
            exit(1);
        }
    }

    VkApplicationInfo applicationInfo = {.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
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
    if (enableValidationLayers)
    {
        extensions[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }

    VkInstanceCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                       .pApplicationInfo = &applicationInfo,
                                       .enabledExtensionCount = glfwExtensionCount,
                                       .ppEnabledExtensionNames = extensions};
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = 1;
        createInfo.enabledExtensionCount = glfwExtensionCount + 1;
        createInfo.ppEnabledLayerNames = layersEnable;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VkInstance instance;
    VkResult result;
    if ((result = vkCreateInstance(&createInfo, NULL, &instance)) != VK_SUCCESS)
    {
        loge("VkResult[%i]: Couldn't create vulkan instance!", result);
    }
    return instance;
}
struct QueueFamilyIndices
{
    int graphics_present;
    int presentation_present;
    int compute_present;
    int raytrace_present;
    uint32_t graphics;
    uint32_t compute;
    uint32_t raytrace;
    uint32_t presentation;
};
VkPhysicalDevice pick_physical_device(VkInstance instance)
{
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
    logi("%i devices found", deviceCount);

    // select discrete gpu as physical device
    VkPhysicalDeviceProperties deviceProperties = {0};
    for (uint32_t i = 0; i < deviceCount; i++)
    {
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            physicalDevice = devices[i];
        }
        vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
        logi("%s | %li", deviceProperties.deviceName, deviceProperties.vendorID);
    }
    if (physicalDevice == VK_NULL_HANDLE)
    {
        loge("No discrete GPU Found");
        exit(1);
    }
    return physicalDevice;
}
struct QueueFamilyIndices get_queue_family(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties queueProps[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueProps);
    struct QueueFamilyIndices fam = {
        .presentation_present = 0, .compute_present = 0, .graphics_present = 0, .raytrace_present = 0};
    for (uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if (queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            fam.graphics_present = 1;
            fam.graphics = i;
        }
        else if (queueProps[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            fam.compute_present = 1;
            fam.compute = i;
        }
        VkBool32 presentSupport = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
        if (presentSupport)
        {
            fam.presentation_present = 1;
            fam.presentation = i;
        }
    }
    return fam;
}

VkDevice create_device(VkPhysicalDevice physicalDevice, struct QueueFamilyIndices queues)
{
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo qs[2];
    int count = 1;
    qs[0] = (VkDeviceQueueCreateInfo){.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                      .queueFamilyIndex = queues.graphics,
                                      .pQueuePriorities = &queuePriority,
                                      .queueCount = 1};
    if (queues.presentation != queues.graphics)
    {
        count++;
        qs[1] = (VkDeviceQueueCreateInfo){.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                          .queueFamilyIndex = queues.presentation,
                                          .pQueuePriorities = &queuePriority,
                                          .queueCount = 1};
    }
    // device extensions
    const char *extensions[1] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkPhysicalDeviceFeatures vkPhysicalDeviceFeatures = {.robustBufferAccess = 0};
    // for old implementations - new implementations ignore layers set here and
    // refer to instance layers
    const char *layersEnable[1] = {"VK_LAYER_KHRONOS_validation"};
    VkDeviceCreateInfo vkDeviceCreateInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
                                             .pQueueCreateInfos = qs,
                                             .queueCreateInfoCount = count,
                                             .ppEnabledLayerNames = layersEnable,
                                             .ppEnabledExtensionNames = extensions,
                                             .enabledExtensionCount = 1,
                                             .enabledLayerCount = 1,
                                             .pEnabledFeatures = &vkPhysicalDeviceFeatures};
    VkDevice device = VK_NULL_HANDLE;
    if (vkCreateDevice(physicalDevice, &vkDeviceCreateInfo, 0, &device) != VK_SUCCESS)
    {
        loge("Couldn't create logical device!");
        exit(1);
    }
    return device;
}

VkSurfaceKHR create_surface(VkInstance instance, GLFWwindow *window)
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(instance, window, 0, &surface) != VK_SUCCESS)
    {
        loge("failed to create window surface!");
    }
    return surface;
}
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    uint32_t formats_count;
    VkPresentModeKHR *presentModes;
    uint32_t presentModes_count;
};

uint32_t clamp(uint32_t d, uint32_t min, uint32_t max)
{
    const uint32_t t = d < min ? min : d;
    return t > max ? max : t;
}
VkSwapchainCreateInfoKHR querySwapChainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface, GLFWwindow *window)
{
    struct SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formats_count, NULL);
    if (details.formats_count != 0)
    {
        details.formats = malloc(sizeof(VkSurfaceFormatKHR) * details.formats_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formats_count, details.formats);
    }

    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.presentModes_count, NULL);
    if (details.presentModes_count != 0)
    {
        details.presentModes = malloc(sizeof(VkSurfaceFormatKHR) * details.formats_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModes_count, details.presentModes);
    }
    int swapChainAdequate = 0;
    swapChainAdequate = !details.formats_count && !details.presentModes_count;
    if (swapChainAdequate)
    {
        loge("Swap chain support not adequeate");
        exit(1);
    }
    // create swapchain
    // choose surface format
    VkSurfaceFormatKHR sformat;
    {
        int found = 0;
        for (uint32_t i = 0; i < details.formats_count; i++)
        {
            if (details.formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
                details.formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                found = 1;
                sformat = details.formats[i];
                break;
            }
        }
        if (!found)
        {
            sformat = details.formats[0];
        }
    }
    VkPresentModeKHR presentMode;
    {
        int found = 0;
        for (uint32_t i = 0; i < details.presentModes_count; i++)
        {
            if (details.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                found = 1;
                presentMode = details.presentModes[i];
                break;
            }
        }
        if (!found)
        {
            presentMode = details.presentModes[0];
        }
    }
    // choose VkExtent2D
    VkExtent2D vkExtent;
    {
        if (details.capabilities.currentExtent.width != UINT32_MAX)
        {
            vkExtent = details.capabilities.currentExtent;
        }
        else
        {
            // upto us to determine swapchain image size
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            vkExtent.width =
                clamp(width, details.capabilities.minImageExtent.width, details.capabilities.maxImageExtent.width);
            vkExtent.height =
                clamp(height, details.capabilities.minImageExtent.height, details.capabilities.maxImageExtent.height);
        }
    }
    uint32_t image_count = details.capabilities.minImageCount + 1;
    if (details.capabilities.maxImageCount > 0 && image_count > details.capabilities.maxImageCount)
    {
        image_count = details.capabilities.maxImageCount;
    }
    logi("Swapchain image count: %li", image_count);
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                                                    .surface = surface,
                                                    .minImageCount = image_count,
                                                    .imageFormat = sformat.format,
                                                    .imageColorSpace = sformat.colorSpace,
                                                    .imageExtent = vkExtent,
                                                    .imageArrayLayers = 1,
                                                    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                    .preTransform = details.capabilities.currentTransform,
                                                    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                                    .presentMode = presentMode,
                                                    .clipped = 1,
                                                    .oldSwapchain = VK_NULL_HANDLE};
    struct QueueFamilyIndices indices = get_queue_family(device, surface);
    if (indices.graphics != indices.presentation)
    {
        uint32_t q[2] = {indices.graphics, indices.presentation};
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = q;
    }
    else
    {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;  // Optional
        swapchainCreateInfo.pQueueFamilyIndices = NULL; // Optional
    }
    return swapchainCreateInfo;
}
VkImageView *getImageViews(VkDevice device, VkFormat format, uint32_t count, VkImage *images)
{
    VkImageView *r = malloc(count * sizeof(VkImageView));
    for (uint32_t i = 0; i < count; i++)
    {
        VkImageViewCreateInfo createInfo =
            (VkImageViewCreateInfo){.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                    .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                    .image = images[i],
                                    .format = format,
                                    .components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                                   VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY},
                                    .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                         .baseMipLevel = 0,
                                                         .levelCount = 1,
                                                         .baseArrayLayer = 0,
                                                         .layerCount = 1}

            };
        if (vkCreateImageView(device, &createInfo, NULL, &r[i]) != VK_SUCCESS)
        {
            loge("failed to create image views!");
        }
    }
    return r;
}

VkShaderModule createShaderModule(VkDevice device, code code)
{
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = code.size, .pCode = code.ptr};
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS)
    {
        loge("failed to create shader module!");
        exit(1);
    }
    return shaderModule;
}
VkRenderPass create_renderpass(VkDevice device, VkFormat format)
{
    VkAttachmentDescription colorAttachment = {.format = format,
                                               .samples = VK_SAMPLE_COUNT_1_BIT,
                                               .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                                               .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                                               .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                                               .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                               .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                                               .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR};
    VkAttachmentReference colorAttachmentRef = {.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass = {.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    .colorAttachmentCount = 1,
                                    .pColorAttachments = &colorAttachmentRef};
    VkRenderPass renderPass;
    VkSubpassDependency dependecy = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo createInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                         .pSubpasses = &subpass,
                                         .subpassCount = 1,
                                         .attachmentCount = 1,
                                         .pAttachments = &colorAttachment,
                                         .dependencyCount = 1,
                                         .pDependencies = &dependecy};
    if (vkCreateRenderPass(device, &createInfo, NULL, &renderPass) != VK_SUCCESS)
    {
        loge("failed to create render pass!");
    }
    return renderPass;
}
VkPipeline createGraphicsPipeline(VkDevice device, VkExtent2D swapchainExtent, VkFormat format)
{
    code vertex = read_shader("vert.spv");
    code frag = read_shader("frag.spv");
    VkShaderModule vertexModule = createShaderModule(device, vertex);
    VkShaderModule fragModule = createShaderModule(device, frag);
    free(vertex.ptr);
    free(frag.ptr);
    vertex.ptr = NULL;
    frag.ptr = NULL;

    VkPipelineShaderStageCreateInfo vertexStage = {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                   .stage = VK_SHADER_STAGE_VERTEX_BIT,
                                                   .module = vertexModule,
                                                   .pName = "main"};
    VkPipelineShaderStageCreateInfo fragStage = {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                                 .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                                                 .module = fragModule,
                                                 .pName = "main"};
    VkPipelineShaderStageCreateInfo shaderStages[2] = {vertexStage, fragStage};

    VkDynamicState dynamicStates[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                                     .dynamicStateCount = (uint32_t)2,
                                                     .pDynamicStates = dynamicStates};
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL};

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE};

    VkViewport viewport = {.x = 0.0f,
                           .y = 0.0f,
                           .width = swapchainExtent.width,
                           .height = swapchainExtent.height,
                           .minDepth = 0.0f,
                           .maxDepth = 1.0f};
    VkRect2D scissor = {.extent = swapchainExtent, .offset = {0, 0}};
    VkPipelineViewportStateCreateInfo viewportState = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                                       .viewportCount = 1,
                                                       .pViewports = &viewport,
                                                       .scissorCount = 1,
                                                       .pScissors = &scissor};
    VkPipelineRasterizationStateCreateInfo rasterizer = {.sType =
                                                             VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                                                         .depthClampEnable = VK_FALSE,
                                                         .rasterizerDiscardEnable = VK_FALSE,
                                                         .polygonMode = VK_POLYGON_MODE_FILL,
                                                         .lineWidth = 1.0f,
                                                         .cullMode = VK_CULL_MODE_BACK_BIT,
                                                         .frontFace = VK_FRONT_FACE_CLOCKWISE,
                                                         .depthBiasEnable = VK_FALSE,
                                                         .depthBiasConstantFactor = 0.0f,
                                                         .depthBiasClamp = 0.0f,
                                                         .depthBiasSlopeFactor = 0.0f};
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,          // Optional
        .pSampleMask = NULL,               // Optional
        .alphaToCoverageEnable = VK_FALSE, // Optional
        .alphaToOneEnable = VK_FALSE       // Optional
    };
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE, // will write to framebuffer as is
    };
    VkPipelineColorBlendStateCreateInfo colorBlending = {.sType =
                                                             VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
                                                         .logicOpEnable = VK_FALSE,
                                                         .attachmentCount = 1,
                                                         .pAttachments = &colorBlendAttachment};
    global.renderPass = create_renderpass(device, format);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .setLayoutCount = 0, .pushConstantRangeCount = 0};
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &global.pipelineLayout) != VK_SUCCESS)
    {
        loge("failed to create pipeline layout!");
    }
    // finally
    VkGraphicsPipelineCreateInfo pipelineInfo = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
                                                 .stageCount = 2,
                                                 .pStages = shaderStages,
                                                 .pVertexInputState = &vertexInputInfo,
                                                 .pInputAssemblyState = &inputAssembly,
                                                 .pViewportState = &viewportState,
                                                 .pRasterizationState = &rasterizer,
                                                 .pMultisampleState = &multisampling,
                                                 .pDepthStencilState = NULL,
                                                 .pColorBlendState = &colorBlending,
                                                 .pDynamicState = &dynamicState,
                                                 .layout = global.pipelineLayout,
                                                 .renderPass = global.renderPass,
                                                 .subpass = 0,
                                                 .basePipelineHandle = VK_NULL_HANDLE,
                                                 .basePipelineIndex = -1};
    VkPipeline graphicsPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline) != VK_SUCCESS)
    {
        loge("failed to create graphics pipeline!");
    }
    vkDestroyShaderModule(device, fragModule, NULL);
    vkDestroyShaderModule(device, vertexModule, NULL);
    return graphicsPipeline;
}

VkFramebuffer *createFrameBuffers(VkDevice device, VkExtent2D swapchainExtent, VkImageView *views,
                                  uint32_t swapchainImages_count)
{

    VkFramebuffer *framebuffers = malloc(sizeof(VkFramebuffer) * swapchainImages_count);
    for (uint32_t i = 0; i < swapchainImages_count; i++)
    {
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = global.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = views;
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffers[i]) != VK_SUCCESS)
        {
            loge("failed to create framebuffer!");
        }
    }
    return framebuffers;
}
VkCommandPool createCommandPool(VkDevice device, struct QueueFamilyIndices q)
{
    VkCommandPool pCommandPool;
    VkCommandPoolCreateInfo poolInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                        .queueFamilyIndex = q.graphics};
    if (vkCreateCommandPool(device, &poolInfo, NULL, &pCommandPool) != VK_SUCCESS)
    {
        loge("Could'nt create command pool for graphics queue");
    }
    return pCommandPool;
}

VkCommandBuffer createCommandBuffer(VkDevice device, VkCommandPool commandPool)
{
    VkCommandBuffer buffer;
    VkCommandBufferAllocateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                                              .commandPool = commandPool,
                                              .commandBufferCount = 1,
                                              .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY};
    if (vkAllocateCommandBuffers(device, &bufferInfo, &buffer) != VK_SUCCESS)
    {
        loge("couldn't allocate command buffer");
    }
    return buffer;
}
void recordCommandBuffer(VkCommandBuffer buffer, VkFramebuffer framebuffer, VkPipeline pipeline, VkExtent2D imageExtent)
{
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = 0, .pInheritanceInfo = NULL};
    if (vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS)
    {
        loge("Couldn't record command buffer");
    }
    VkClearValue clearColor = {{{0, 0, 0, 1}}};
    VkRenderPassBeginInfo rBeginInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                                        .renderPass = global.renderPass,
                                        .renderArea = {.offset = {0, 0}, .extent = imageExtent},
                                        .framebuffer = framebuffer,
                                        .clearValueCount = 1,
                                        .pClearValues = &clearColor};
    vkCmdBeginRenderPass(buffer, &rBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    // bind stuff
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    VkViewport viewport = {.x = 0.0f,
                           .y = 0.0f,
                           .width = (float)(imageExtent.width),
                           .height = (float)(imageExtent.height),
                           .minDepth = 0.0f,
                           .maxDepth = 1.0f};
    vkCmdSetViewport(buffer, 0, 1, &viewport);

    VkRect2D scissor = {.offset = {0, 0}, .extent = imageExtent};
    vkCmdSetScissor(buffer, 0, 1, &scissor);

    // thank finally god
    vkCmdDraw(buffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(buffer);
    if (vkEndCommandBuffer(buffer) != VK_SUCCESS)
    {
        loge("Failed to record command buffer");
    }
}
VkSemaphore createSemaphore(VkDevice device)
{
    VkSemaphore waitForAcquire;
    if (vkCreateSemaphore(device, &(VkSemaphoreCreateInfo){.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO}, NULL,
                          &waitForAcquire) != VK_SUCCESS)
    {
        loge("falied to create semaphore");
    }
    return waitForAcquire;
}
VkFence createFence(VkDevice device)
{
    VkFence waitForAcquire;
    if (vkCreateFence(
            device,
            &(VkFenceCreateInfo){.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .flags = VK_FENCE_CREATE_SIGNALED_BIT},
            NULL, &waitForAcquire) != VK_SUCCESS)
    {
        loge("falied to create fence");
    }
    return waitForAcquire;
}

int main()
{
    init_glfw();
    GLFWwindow *window = create_glfw_window(1920, 1080, "Vulkan window");

    VkInstance instance = createInstance();
    VkSurfaceKHR surface = create_surface(instance, window);
    VkDebugUtilsMessengerEXT debugMessenger = createDebugMessenger(instance);
    VkPhysicalDevice physicalDevice = pick_physical_device(instance);
    struct QueueFamilyIndices queues = get_queue_family(physicalDevice, surface);
    VkDevice device = create_device(physicalDevice, queues);
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queues.graphics, 0, &graphicsQueue);
    VkQueue presentQueue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queues.presentation, 0, &presentQueue);

    // swap chain
    VkSwapchainCreateInfoKHR vkSwapChainCreateInfo = querySwapChainSupportDetails(physicalDevice, surface, window);
    VkSwapchainKHR swapchain;
    if (vkCreateSwapchainKHR(device, &vkSwapChainCreateInfo, NULL, &swapchain) != VK_SUCCESS)
    {
        loge("failed to create swap chain!");
    }
    // get swap chain images
    VkImage *swapchainImages;
    uint32_t swapchainImages_count;
    {
        vkGetSwapchainImagesKHR(device, swapchain, &swapchainImages_count, NULL);
        swapchainImages = malloc(sizeof(VkImage) * swapchainImages_count);
        vkGetSwapchainImagesKHR(device, swapchain, &swapchainImages_count, swapchainImages);
    }
    VkImageView *swapchainImageViews =
        getImageViews(device, vkSwapChainCreateInfo.imageFormat, swapchainImages_count, swapchainImages);

    VkPipeline graphicsPipeline =
        createGraphicsPipeline(device, vkSwapChainCreateInfo.imageExtent, vkSwapChainCreateInfo.imageFormat);
    VkFramebuffer *framebuffers =
        createFrameBuffers(device, vkSwapChainCreateInfo.imageExtent, swapchainImageViews, swapchainImages_count);
    (void)(framebuffers);

    // create command pools
    VkCommandPool commandPool = createCommandPool(device, queues);
    VkCommandBuffer buffer = createCommandBuffer(device, commandPool);
    recordCommandBuffer(buffer, framebuffers[0], graphicsPipeline, vkSwapChainCreateInfo.imageExtent);

    VkFence inFlightFence = createFence(device);                   // signaled when frame presentation is finished
    VkSemaphore imageAvailableSemaphore = createSemaphore(device); // signaled when image aquired from swapchain
    VkSemaphore renderFinishSemaphore = createSemaphore(device);   // signaled when image is completely drawn into
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        // draw
        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFence);
        uint32_t i;
        vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &i);
        vkResetCommandBuffer(buffer, 0);
        recordCommandBuffer(buffer, framebuffers[i], graphicsPipeline, vkSwapChainCreateInfo.imageExtent);
        VkPipelineStageFlags stages[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submitInfo = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                                   .waitSemaphoreCount = 1,
                                   .pWaitSemaphores = &imageAvailableSemaphore,
                                   .pWaitDstStageMask = stages,
                                   .commandBufferCount = 1,
                                   .pCommandBuffers = &buffer,
                                   .signalSemaphoreCount = 1,
                                   .pSignalSemaphores = &renderFinishSemaphore

        };
        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
        {
            loge("Couldn't submit cmd buffer to queue!");
        }
        // present
        VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pWaitSemaphores = &renderFinishSemaphore,
            .waitSemaphoreCount = 1,
            .pImageIndices = &i,
        };
        vkQueuePresentKHR(presentQueue, &presentInfo);
    }
    vkDeviceWaitIdle(device);
    // cleanup
    vkDestroySemaphore(device, imageAvailableSemaphore, NULL);
    vkDestroySemaphore(device, renderFinishSemaphore, NULL);
    vkDestroyFence(device, inFlightFence, NULL);
    vkDestroyCommandPool(device, commandPool, NULL);
    vkDestroyPipeline(device, graphicsPipeline, NULL);
    for (uint32_t i = 0; i < swapchainImages_count; i++)
    {
        vkDestroyFramebuffer(device, framebuffers[i], NULL);
        vkDestroyImageView(device, swapchainImageViews[i], NULL);
    }
    vkDestroyRenderPass(device, global.renderPass, NULL);
    vkDestroyPipelineLayout(device, global.pipelineLayout, NULL);
    vkDestroySwapchainKHR(device, swapchain, NULL);
    vkDestroyDevice(device, 0);
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, NULL);
    vkDestroySurfaceKHR(instance, surface, 0);
    vkDestroyInstance(instance, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
