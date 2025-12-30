#include "engine/tVulkanDevice.h"

#include <array>
#include <cstring>
#include <vector>

#include <vulkan/vulkan.hpp>

constexpr std::array<const char *, 4> DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                           VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
                                                           VK_EXT_MESH_SHADER_EXTENSION_NAME,
                                                           VK_KHR_MAINTENANCE_4_EXTENSION_NAME};

tVulkanDevice::~tVulkanDevice()
{
    if (TracyContext != nullptr)
    {
        TracyVkDestroy(TracyContext);
        TracyContext = nullptr;
    }
    spdlog::info("tVulkanDevice: Destroyed");
}

void tVulkanDevice::init(const vk::raii::Instance &instance, const vk::SurfaceKHR &surface, const bool enableValidation)
{
    spdlog::info("tVulkanDevice: Initializing...");
    ValidationEnabled = enableValidation;
    Surface = surface;
    pickPhysicalDevice(instance);
    createLogicalDevice();
    createCommandPool();
    initTracyContext();
    spdlog::info("tVulkanDevice: Initialized");
}

vk::raii::CommandBuffer tVulkanDevice::beginSingleTimeCommands() const
{
    spdlog::trace("tVulkanDevice: Starting single time command...");
    vk::CommandBufferAllocateInfo cbai(*CommandPool, vk::CommandBufferLevel::ePrimary, 1);
    vk::raii::CommandBuffers commandBuffers(Device, cbai);
    vk::raii::CommandBuffer commandBuffer = std::move(commandBuffers[0]);

    vk::CommandBufferBeginInfo cbbi{};
    cbbi.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    commandBuffer.begin(cbbi);

    spdlog::trace("tVulkanDevice: Started single time command");
    return commandBuffer;
}

void tVulkanDevice::endSingleTimeCommands(vk::raii::CommandBuffer &commandBuffer) const
{
    spdlog::trace("tVulkanDevice: Ending single time command...");
    commandBuffer.end();
    vk::CommandBufferSubmitInfo cbsi{};
    cbsi.commandBuffer = commandBuffer;

    vk::SubmitInfo2 si{};
    si.commandBufferInfoCount = 1;
    si.pCommandBufferInfos = &cbsi;

    Queue.submit2(si);
    Queue.waitIdle();
    commandBuffer.clear();
    spdlog::trace("tVulkanDevice: Ended single time command");
}

bool tVulkanDevice::supportsRequiredFeaturesAndExtensions(vk::PhysicalDevice device) const
{
    const auto props = device.getProperties2();
    const auto deviceName = props.properties.deviceName.data();

    const auto extensions = device.enumerateDeviceExtensionProperties();
    auto hasExtension = [&extensions](const char *name) {
        for (const auto &ext : extensions)
        {
            if (std::strcmp(ext.extensionName, name) == 0)
                return true;
        }
        return false;
    };

    for (const auto *required : DEVICE_EXTENSIONS)
    {
        if (!hasExtension(required))
        {
            spdlog::info("tVulkanDevice: Skipping device {} (missing extension {})", deviceName, required);
            return false;
        }
    }

    const auto featuresChain = device.getFeatures2<vk::PhysicalDeviceFeatures2,
                                                   vk::PhysicalDeviceMeshShaderFeaturesEXT,
                                                   vk::PhysicalDeviceMaintenance4Features,
                                                   vk::PhysicalDeviceBufferDeviceAddressFeatures,
                                                   vk::PhysicalDeviceTimelineSemaphoreFeatures,
                                                   vk::PhysicalDeviceSynchronization2Features,
                                                   vk::PhysicalDeviceDynamicRenderingFeatures>();

    const auto &meshFeatures = featuresChain.get<vk::PhysicalDeviceMeshShaderFeaturesEXT>();
    if (!meshFeatures.meshShader || !meshFeatures.taskShader)
    {
        spdlog::info("tVulkanDevice: Skipping device {} (mesh/task shader features unsupported)", deviceName);
        return false;
    }

    const auto &m4Features = featuresChain.get<vk::PhysicalDeviceMaintenance4Features>();
    if (!m4Features.maintenance4)
    {
        spdlog::info("tVulkanDevice: Skipping device {} (maintenance4 extension unsupported)", deviceName);
        return false;
    }

    const auto &bdaFeatures = featuresChain.get<vk::PhysicalDeviceBufferDeviceAddressFeatures>();
    if (!bdaFeatures.bufferDeviceAddress)
    {
        spdlog::info("tVulkanDevice: Skipping device {} (buffer device address unsupported)", deviceName);
        return false;
    }

    const auto &timelineFeatures = featuresChain.get<vk::PhysicalDeviceTimelineSemaphoreFeatures>();
    if (!timelineFeatures.timelineSemaphore)
    {
        spdlog::info("tVulkanDevice: Skipping device {} (timeline semaphore unsupported)", deviceName);
        return false;
    }

    const auto &sync2Features = featuresChain.get<vk::PhysicalDeviceSynchronization2Features>();
    if (!sync2Features.synchronization2)
    {
        spdlog::info("tVulkanDevice: Skipping device {} (synchronization2 unsupported)", deviceName);
        return false;
    }

    const auto &drFeatures = featuresChain.get<vk::PhysicalDeviceDynamicRenderingFeatures>();
    if (!drFeatures.dynamicRendering)
    {
        spdlog::info("tVulkanDevice: Skipping device {} (dynamic rendering unsupported)", deviceName);
        return false;
    }

    return true;
}

void tVulkanDevice::pickPhysicalDevice(const vk::raii::Instance &instance)
{
    spdlog::info("tVulkanDevice: Selecting physical device...");
    auto physicalDevices = instance.enumeratePhysicalDevices();
    if (physicalDevices.empty())
        throw std::runtime_error("No Vulkan-compatible GPUs found.");

    for (const auto &device : physicalDevices)
    {
        const auto props = device.getProperties2();
        if (!supportsRequiredFeaturesAndExtensions(device))
            continue;

        const auto queueFamilies = device.getQueueFamilyProperties();
        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
        {
            const auto &qf = queueFamilies[i];
            bool hasPresent = queueSupportsPresent(device, i, Surface);

            if ((qf.queueFlags & vk::QueueFlagBits::eGraphics) && (qf.queueFlags & vk::QueueFlagBits::eCompute) &&
                hasPresent)
            {
                PhysicalDevice = vk::raii::PhysicalDevice(device);
                QueueFamily = i;
                spdlog::info("tVulkanDevice: Selected device {} (family {}). Supports Graphics, Compute and Present",
                             props.properties.deviceName.data(),
                             i);
                return;
            }
        }
    }

    throw std::runtime_error("No suitable GPU with GRAPHICS+COMPUTE+PRESENT found.");
}

void tVulkanDevice::createLogicalDevice()
{
    spdlog::info("tVulkanDevice: Creating logical device...");
    float priority = 1.0f;

    vk::DeviceQueueCreateInfo qci;
    qci.setQueueFamilyIndex(QueueFamily).setQueueCount(1).setPQueuePriorities(&priority);

    vk::DeviceCreateInfo dci{};
    vk::PhysicalDeviceMeshShaderFeaturesEXT msf{};
    msf.meshShader = VK_TRUE;
    msf.taskShader = VK_TRUE;
    vk::PhysicalDeviceTimelineSemaphoreFeatures tsf{VK_TRUE};
    vk::PhysicalDeviceSynchronization2Features s2f{VK_TRUE};
    vk::PhysicalDeviceDynamicRenderingFeatures drf{VK_TRUE};
    vk::PhysicalDeviceMaintenance4Features m4f{};
    m4f.maintenance4 = VK_TRUE;
    vk::PhysicalDeviceBufferDeviceAddressFeatures bdaf{};
    bdaf.bufferDeviceAddress = VK_TRUE;
    tsf.pNext = &msf;
    s2f.pNext = &tsf;
    drf.pNext = &s2f;
    m4f.pNext = &drf;
    bdaf.pNext = &m4f;
    dci.setQueueCreateInfos(qci).setPEnabledExtensionNames(DEVICE_EXTENSIONS);
    dci.pNext = &bdaf;

    Device = vk::raii::Device(PhysicalDevice, dci);
    Queue = Device.getQueue(QueueFamily, 0);
    spdlog::info("tVulkanDevice: Created logical device");
}

void tVulkanDevice::createCommandPool()
{
    spdlog::info("tVulkanDevice: Creating command pool...");
    vk::CommandPoolCreateInfo cpci({vk::CommandPoolCreateFlagBits::eResetCommandBuffer}, QueueFamily);
    CommandPool = vk::raii::CommandPool(Device, cpci);
    spdlog::info("tVulkanDevice: Command pool created");
}

void tVulkanDevice::initTracyContext()
{
#if defined(TRACY_ENABLE)
    vk::CommandBufferAllocateInfo cbai(*CommandPool, vk::CommandBufferLevel::ePrimary, 1);
    vk::raii::CommandBuffers commandBuffers(Device, cbai);
    TracyCommandBuffer = std::move(commandBuffers[0]);

    TracyContext = TracyVkContext(*PhysicalDevice, *Device, *Queue, *TracyCommandBuffer);
    if (TracyContext != nullptr)
    {
        constexpr char name[] = "MainQueue";
        TracyVkContextName(TracyContext, name, sizeof(name) - 1);
    }
#endif
}

bool tVulkanDevice::queueSupportsPresent(vk::PhysicalDevice device, uint32_t queueFamily, vk::SurfaceKHR surface)
{
    return device.getSurfaceSupportKHR(queueFamily, surface);
}
