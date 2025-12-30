#pragma once

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
// vulkan-tracy include order
#include <tracy/TracyVulkan.hpp>

class tSim;
class tCamera;
class tGui;
class tSwapchain;
class tVulkanDevice;

class tRenderer
{
  public:
    tRenderer(const tCamera &camera, const tGui &gui, const tVulkanDevice &Device, tSwapchain &Swapchain, tSim &sim);
    ~tRenderer() { spdlog::info("tRenderer: Destroyed"); }

    void drawFrame();
    void recreateSwapchain(vk::Extent2D extent);

  private:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    void initSwapchainLayouts();
    void createGraphicsPipeline();
    void createSyncObjects();
    void createShaderModules();
    void createCommandBuffers();
    void createCommandPool();

    std::pair<vk::Result, uint32_t> acquireNextImage();
    std::pair<vk::Result, uint32_t> synchronizeFrame();
    void submit(uint32_t ixImage);
    void present(uint32_t ixImage);
    void recordReusableCommandBuffers();
    void recordPerFrameCommandBuffers(uint32_t ixImage);
    void recordGraphicsCommandBuffer(uint32_t ixImage);
    void recordGraphicsCommandBufferNoop(uint32_t ixImage) {};
    void recordGraphicsPass(const vk::raii::CommandBuffer &commandBuffer,
                            const vk::Image &image,
                            const vk::raii::ImageView &imageView);
    void waitTimelineValue(uint64_t value);

    const tCamera &Camera;
    const tVulkanDevice &Device;
    const tGui &Gui;
    const vk::raii::Device &LogicalDevice;
    const vk::raii::PhysicalDevice &PhysicalDevice;
    const vk::raii::Queue &Queue;
    tSwapchain &Swapchain;
    tSim &Sim;

    using RecordGraphicsFn = void (tRenderer::*)(uint32_t);
    const TracyVkCtx TracyContext;
    const RecordGraphicsFn RecordGraphicsPerFrame;

    vk::raii::PipelineLayout GraphicsPipelineLayout{nullptr};
    vk::raii::Pipeline GraphicsPipeline{nullptr};

    vk::raii::CommandPool CommandPool{nullptr};
    vk::raii::CommandBuffers CommandBuffers{nullptr};
    vk::raii::CommandBuffers GuiCommandBuffers{nullptr};
    vk::raii::CommandBuffers SimCommandBuffers{nullptr};

    vk::raii::ShaderModule TaskShaderModule{nullptr};
    vk::raii::ShaderModule MeshShaderModule{nullptr};
    vk::raii::ShaderModule FragmentShaderModule{nullptr};

    std::vector<vk::raii::Semaphore> ImageAvailable;
    std::vector<vk::raii::Semaphore> RenderFinished;
    vk::raii::Semaphore FrameTimeline{nullptr};
    std::vector<uint64_t> FrameTimelineValues;
    std::vector<uint64_t> ImageTimelineValues;
    uint64_t LastTimelineValue{0};
    size_t IxCurrentFrame{0};

    vk::MemoryBarrier2 ComputeToGraphicsBarrier{vk::PipelineStageFlagBits2::eComputeShader,
                                                vk::AccessFlagBits2::eShaderWrite,
                                                vk::PipelineStageFlagBits2::eMeshShaderEXT,
                                                vk::AccessFlagBits2::eShaderRead};
};
