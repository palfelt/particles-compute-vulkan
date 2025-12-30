#include "engine/tSwapchain.h"

#include <algorithm>
#include <stdexcept>

#include "engine/tVulkanDevice.h"
#include "helpers/memoryAllocation.h"

namespace
{
static uint32_t clampu32(uint32_t v, uint32_t lo, uint32_t hi)
{
    return std::max(lo, std::min(v, hi));
}
} // namespace

void tSwapchain::init(const vk::raii::Instance &instance,
                      const tVulkanDevice &device,
                      const vk::raii::SurfaceKHR &surface,
                      const vk::Extent2D extent)
{
    spdlog::info("tSwapchain: Initializing...");
    Instance = &instance;
    Device = &device.getLogicalDevice();
    PhysicalDevice = &device.getPhysicalDevice();
    Queue = &device.getQueue();
    Surface = &surface;

    create(extent);
    spdlog::info("tSwapchain: Initialized");
}

void tSwapchain::recreate(vk::Extent2D extent)
{
    spdlog::info("tSwapchain: Recreating...");
    if (Swapchain == nullptr)
        throw std::runtime_error("Trying to recreate swapchain before it was created");

    // Order matters w.r.t. destruction
    ImageViews.clear();
    DepthBuffer = {};
    Swapchain = nullptr;

    create(extent);
    spdlog::info("tSwapchain: Recreated");
}

void tSwapchain::create(vk::Extent2D extent)
{
    createSwapchain(extent);
    createImageViews();
    createDepthResources();
}

void tSwapchain::createSwapchain(vk::Extent2D extent)
{
    spdlog::info("tSwapchain: Creating SwapchainKHR...");
    auto sc = PhysicalDevice->getSurfaceCapabilitiesKHR(*Surface);
    auto sf = PhysicalDevice->getSurfaceFormatsKHR(*Surface);

    auto chosen = sf[0];
    for (auto &f : sf)
    {
        if (f.format == vk::Format::eB8G8R8A8Unorm && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            chosen = f;
            break;
        }
    }
    ColorFormat = chosen.format;
    Extent.width = clampu32(extent.width, sc.minImageExtent.width, sc.maxImageExtent.width);
    Extent.height = clampu32(extent.height, sc.minImageExtent.height, sc.maxImageExtent.height);

    MinImageCount = std::max(2u, sc.minImageCount);
    if (sc.maxImageCount > 0 && MinImageCount > sc.maxImageCount)
        MinImageCount = sc.maxImageCount;

    vk::SwapchainCreateInfoKHR sci{};
    sci.surface = *Surface;
    sci.minImageCount = MinImageCount;
    sci.imageFormat = ColorFormat;
    sci.imageColorSpace = chosen.colorSpace;
    sci.imageExtent = Extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    sci.imageSharingMode = vk::SharingMode::eExclusive;
    sci.preTransform = sc.currentTransform;
    sci.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    sci.presentMode = vk::PresentModeKHR::eImmediate;
    sci.clipped = VK_TRUE;

    Swapchain = Device->createSwapchainKHR(sci);
    spdlog::info("tSwapchain: SwapchainKHR created with extent {}x{}", Extent.width, Extent.height);
}

void tSwapchain::createImageViews()
{
    spdlog::info("tSwapchain: Creating image views...");
    Images = Swapchain.getImages();
    spdlog::trace("tSwapchain: Got {} images", Images.size());

    ImageViews.reserve(Images.size());
    vk::ImageViewCreateInfo ivci(
        {}, {}, vk::ImageViewType::e2D, ColorFormat, {}, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
    for (auto image : Images)
    {
        ivci.image = image;
        ImageViews.push_back(Device->createImageView(ivci));
    }
    spdlog::info("tSwapchain: Image views created");
}

void tSwapchain::createDepthResources()
{
    spdlog::info("tSwapchain: Creating depth resources...");
    vk::ImageCreateInfo ici({},
                            vk::ImageType::e2D,
                            DepthFormat,
                            vk::Extent3D(Extent.width, Extent.height, 1),
                            1,
                            1,
                            vk::SampleCountFlagBits::e1,
                            vk::ImageTiling::eOptimal,
                            vk::ImageUsageFlagBits::eDepthStencilAttachment,
                            vk::SharingMode::eExclusive,
                            0,
                            nullptr,
                            vk::ImageLayout::eUndefined);

    DepthBuffer.Image = Device->createImage(ici);

    const auto allocInfo = getMemoryAllocateInfo(
        *PhysicalDevice, DepthBuffer.Image.getMemoryRequirements(), vk::MemoryPropertyFlagBits::eDeviceLocal);
    DepthBuffer.Memory = Device->allocateMemory(allocInfo);
    DepthBuffer.Image.bindMemory(DepthBuffer.Memory, 0);

    vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eDepth;
    if (DepthFormat == vk::Format::eD32SfloatS8Uint || DepthFormat == vk::Format::eD24UnormS8Uint)
    {
        aspect |= vk::ImageAspectFlagBits::eStencil;
    }

    vk::ImageViewCreateInfo vi({},
                               DepthBuffer.Image,
                               vk::ImageViewType::e2D,
                               DepthFormat,
                               vk::ComponentMapping(),
                               vk::ImageSubresourceRange(aspect, 0, 1, 0, 1));

    DepthBuffer.ImageView = Device->createImageView(vi);
    spdlog::info("tSwapchain: Created depth buffer resources");
}
