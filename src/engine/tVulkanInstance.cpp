#include "engine/tVulkanInstance.h"

#include <cstring>
#include <stdexcept>
#include <vector>

#include <GLFW/glfw3.h>

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                      vk::DebugUtilsMessageTypeFlagsEXT messageType,
                                                      const vk::DebugUtilsMessengerCallbackDataEXT *callbackData,
                                                      void *userData)
{
    (void)messageType;
    (void)userData;

    if (callbackData->pMessageIdName &&
        (strstr(callbackData->pMessageIdName, "Loader") || strstr(callbackData->pMessageIdName, "loader") ||
         strstr(callbackData->pMessage, "vk_loader_settings.json")))
    {
        return VK_FALSE;
    }

    if (messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo ||
        messageSeverity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose)
    {
        return VK_FALSE;
    }

    switch (messageSeverity)
    {
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        spdlog::debug("[VULKAN VERBOSE]: {}", callbackData->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
        spdlog::info("[VULKAN INFO]: {}", callbackData->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
        spdlog::warn("[VULKAN WARNING]: {}", callbackData->pMessage);
        break;
    case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
        spdlog::error("[VULKAN ERROR]: {}", callbackData->pMessage);
        break;
    default:
        spdlog::critical("[VULKAN]: {}", callbackData->pMessage);
        break;
    }

    return VK_FALSE;
}

static vk::DebugUtilsMessengerCreateInfoEXT makeDebugMessengerCreateInfo()
{
    return vk::DebugUtilsMessengerCreateInfoEXT{
        {},
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        debugCallback};
}

tVulkanInstance::tVulkanInstance(const bool enableValidation)
{
    spdlog::info("tVulkanInstance: Initializing...");
    if (enableValidation && !checkValidationLayerSupport())
    {
        throw std::runtime_error("Validation layers requested, but not available!");
    }

    vk::ApplicationInfo appInfo{"vulkan-compute",
                                VK_MAKE_VERSION(1, 0, 0),
                                "vulkan-compute-engine",
                                VK_MAKE_VERSION(1, 0, 0),
                                VK_API_VERSION_1_4};

    auto extensions = getRequiredExtensions(enableValidation);
    auto layers =
        enableValidation ? std::vector<const char *>{"VK_LAYER_KHRONOS_validation"} : std::vector<const char *>{};

    vk::InstanceCreateInfo ici{};
    ici.setPApplicationInfo(&appInfo).setPEnabledExtensionNames(extensions).setPEnabledLayerNames(layers);

    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidation)
    {
        debugCreateInfo = makeDebugMessengerCreateInfo();
        ici.setPNext(&debugCreateInfo);
    }

    Instance = vk::raii::Instance(Context, ici);

    Loader = vk::detail::DispatchLoaderDynamic(*Instance, vkGetInstanceProcAddr);
    if (enableValidation)
    {
        DebugMessenger = Instance.createDebugUtilsMessengerEXT(makeDebugMessengerCreateInfo());
    }
    spdlog::info("tVulkanInstance: Initialized (validation {})", (enableValidation ? "ON" : "OFF"));
}

bool tVulkanInstance::checkValidationLayerSupport()
{
    auto availableLayers = vk::enumerateInstanceLayerProperties();
    for (const auto &layer : availableLayers)
    {
        if (strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") == 0)
        {
            spdlog::trace("tVulkanInstance: ValidationLayer is supported");
            return true;
        }
    }

    spdlog::warn("tVulkanInstance: ValidationLayer not supported");
    return false;
}

std::vector<const char *> tVulkanInstance::getRequiredExtensions(const bool enableValidation)
{
    if (!glfwInit())
    {
        spdlog::warn("GLFW not initialized before getRequiredExtensions() — extensions may be empty");
        return {};
    }

    uint32_t glfwExtCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtCount);
    std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtCount);
    if (enableValidation)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}
