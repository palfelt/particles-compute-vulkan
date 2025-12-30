#include "engine/tRenderer.h"

#include <cstring>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <glm/gtc/matrix_transform.hpp>
#include <tracy/Tracy.hpp>

#include "engine/tCamera.h"
#include "engine/tGui.h"
#include "engine/tSwapchain.h"
#include "engine/tVulkanDevice.h"
#include "helpers/loadShaders.h"
#include "sim/constants.h"
#include "sim/tSim.h"

namespace
{
struct ParticlePushConstants
{
    vk::DeviceAddress particles;
    uint32_t numParticles;
    uint32_t pad0;
    uint32_t pad1;
    uint32_t pad2;
};
} // namespace

tRenderer::tRenderer(
    const tCamera &camera, const tGui &gui, const tVulkanDevice &device, tSwapchain &swapchain, tSim &sim)
    : Camera(camera), Gui(gui), Device(device), LogicalDevice(device.getLogicalDevice()),
      PhysicalDevice(device.getPhysicalDevice()), Queue(device.getQueue()), Swapchain(swapchain), Sim(sim),
      TracyContext(Device.getTracyContext()),
      RecordGraphicsPerFrame(TracyContext != nullptr ? &tRenderer::recordGraphicsCommandBuffer
                                                     : &tRenderer::recordGraphicsCommandBufferNoop)
{
    spdlog::info("tRenderer: Initializing...");
    initSwapchainLayouts();
    createShaderModules();
    createGraphicsPipeline();
    createCommandPool();
    createCommandBuffers();
    createSyncObjects();
    recordReusableCommandBuffers();
    spdlog::info("tRenderer: Initialized");
}

void tRenderer::drawFrame()
{
    ZoneScopedN("tRenderer: drawFrame()");
    spdlog::trace("tRenderer: Drawing frame {}", IxCurrentFrame);
    const auto syncResults = synchronizeFrame();
    const auto result = syncResults.first;
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        return;
    }

    const auto ixImage = syncResults.second;
    recordPerFrameCommandBuffers(ixImage);
    submit(ixImage);
    present(ixImage);
    spdlog::trace("tRenderer: Finished drawing frame {}, image {}", IxCurrentFrame, ixImage);
    IxCurrentFrame = (IxCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void tRenderer::recreateSwapchain(const vk::Extent2D extent)
{
    ZoneScopedN("tRenderer: recreateSwapchain()");
    spdlog::info("tRenderer: Recreating swapchain...");
    waitTimelineValue(LastTimelineValue);
    Queue.waitIdle();

    Swapchain.recreate(extent);
    initSwapchainLayouts();
    createGraphicsPipeline();
    createCommandBuffers();
    createSyncObjects();
    recordReusableCommandBuffers();

    IxCurrentFrame = 0;
    spdlog::info("tRenderer: Swapchain recreated");
};

void tRenderer::initSwapchainLayouts()
{
    spdlog::info("tRenderer: Creating initial image layouts ...");
    auto images = Swapchain.getImages();
    auto commandBuffer = Device.beginSingleTimeCommands();

    for (auto image : images)
    {
        vk::ImageMemoryBarrier2 barrier{};
        barrier.srcStageMask = vk::PipelineStageFlagBits2::eTopOfPipe;
        barrier.srcAccessMask = vk::AccessFlagBits2::eNone;
        barrier.dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe;
        barrier.dstAccessMask = vk::AccessFlagBits2::eNone;
        barrier.oldLayout = vk::ImageLayout::eUndefined;
        barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
        barrier.image = image;
        barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

        commandBuffer.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, barrier));
    }

    spdlog::info("tRenderer: Initial image layouts created");
    Device.endSingleTimeCommands(commandBuffer);
}

void tRenderer::createGraphicsPipeline()
{
    spdlog::info("tRenderer: Creating graphics pipeline...");
    std::array setLayouts{*Sim.getDescriptorSetLayout(), *Camera.getDescriptorSetLayout()};

    vk::PushConstantRange particlePcRange{
        vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT, 0, sizeof(ParticlePushConstants)};
    vk::PipelineLayoutCreateInfo plci({}, setLayouts, particlePcRange);

    GraphicsPipelineLayout = vk::raii::PipelineLayout(LogicalDevice, plci);

    vk::PipelineShaderStageCreateInfo tsci{};
    tsci.stage = vk::ShaderStageFlagBits::eTaskEXT;
    tsci.module = *TaskShaderModule;
    tsci.pName = "main";

    vk::PipelineShaderStageCreateInfo msci{};
    msci.stage = vk::ShaderStageFlagBits::eMeshEXT;
    msci.module = *MeshShaderModule;
    msci.pName = "main";

    vk::PipelineShaderStageCreateInfo fsci{};
    fsci.stage = vk::ShaderStageFlagBits::eFragment;
    fsci.module = *FragmentShaderModule;
    fsci.pName = "main";

    std::array stages{tsci, msci, fsci};

    const auto extent = Swapchain.getExtent();
    vk::Viewport vp{0.0f, 0.0f, float(extent.width), float(extent.height), 0.0f, 1.0f};
    vk::Rect2D sc{{0, 0}, extent};

    vk::PipelineViewportStateCreateInfo vpState{};
    vpState.viewportCount = 1;
    vpState.pViewports = &vp;
    vpState.scissorCount = 1;
    vpState.pScissors = &sc;

    vk::PipelineRasterizationStateCreateInfo rs{};
    rs.polygonMode = vk::PolygonMode::eFill;
    rs.cullMode = vk::CullModeFlagBits::eBack;
    rs.frontFace = vk::FrontFace::eClockwise;
    rs.lineWidth = 1.0f;

    vk::PipelineMultisampleStateCreateInfo ms{};
    ms.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineDepthStencilStateCreateInfo ds{};
    ds.depthTestEnable = VK_FALSE;
    ds.depthWriteEnable = VK_FALSE;

    vk::PipelineColorBlendAttachmentState cbAtt{};
    cbAtt.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                           vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

    vk::PipelineColorBlendStateCreateInfo cb{};
    cb.attachmentCount = 1;
    cb.pAttachments = &cbAtt;

    vk::PipelineRenderingCreateInfoKHR prci{};
    prci.pNext = VK_NULL_HANDLE;
    prci.colorAttachmentCount = 1;
    prci.pColorAttachmentFormats = &Swapchain.getColorFormat();
    prci.depthAttachmentFormat = Swapchain.getDepthFormat();

    vk::GraphicsPipelineCreateInfo gpi{};
    gpi.pNext = &prci;
    gpi.stageCount = static_cast<uint32_t>(stages.size());
    gpi.pStages = stages.data();
    gpi.pVertexInputState = nullptr;
    gpi.pInputAssemblyState = nullptr;
    gpi.pViewportState = &vpState;
    gpi.pRasterizationState = &rs;
    gpi.pMultisampleState = &ms;
    gpi.pDepthStencilState = &ds;
    gpi.pColorBlendState = &cb;
    gpi.layout = *GraphicsPipelineLayout;
    gpi.renderPass = VK_NULL_HANDLE; // dynamic rendering
    gpi.subpass = 0;

    GraphicsPipeline = vk::raii::Pipeline{LogicalDevice, nullptr, gpi};
    spdlog::info("tRenderer: Graphics pipeline created");
}

void tRenderer::createSyncObjects()
{
    spdlog::info("tRenderer: Creating sync objects...");
    // Per-frame
    ImageAvailable.clear();
    ImageAvailable.reserve(MAX_FRAMES_IN_FLIGHT);

    vk::SemaphoreCreateInfo semci{};

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        ImageAvailable.emplace_back(LogicalDevice, semci);
    }

    // Per-image
    const uint32_t imageCount = Swapchain.getImageCount();
    RenderFinished.clear();
    RenderFinished.reserve(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i)
    {
        RenderFinished.emplace_back(LogicalDevice, semci);
    }

    vk::SemaphoreTypeCreateInfo timelineTypeInfo{vk::SemaphoreType::eTimeline};
    vk::SemaphoreCreateInfo timelineCreateInfo{{}, &timelineTypeInfo};
    FrameTimeline = vk::raii::Semaphore(LogicalDevice, timelineCreateInfo);
    FrameTimelineValues.assign(MAX_FRAMES_IN_FLIGHT, 0);
    ImageTimelineValues.assign(imageCount, 0);
    LastTimelineValue = 0;

    spdlog::info("tRenderer: Created {} ImageAvailable semaphores", ImageAvailable.size());
    spdlog::info("tRenderer: Created {} RenderFinished semaphores", RenderFinished.size());
    spdlog::info("tRenderer: Created timeline semaphore for frame sync");
}

void tRenderer::createShaderModules()
{
    spdlog::info("tRenderer: Creating shader modules...");
    TaskShaderModule = loadShaderModule(LogicalDevice, "task.task.spv");
    MeshShaderModule = loadShaderModule(LogicalDevice, "mesh.mesh.spv");
    FragmentShaderModule = loadShaderModule(LogicalDevice, "fragment.frag.spv");
    spdlog::info("tRenderer: Shader modules created");
}

void tRenderer::createCommandBuffers()
{
    spdlog::info("tRenderer: Creating command buffers...");
    CommandBuffers.clear();
    CommandBuffers = vk::raii::CommandBuffers(
        LogicalDevice, {CommandPool, vk::CommandBufferLevel::ePrimary, Swapchain.getImageCount()});
    GuiCommandBuffers = vk::raii::CommandBuffers(
        LogicalDevice, {CommandPool, vk::CommandBufferLevel::ePrimary, Swapchain.getImageCount()});
    SimCommandBuffers = vk::raii::CommandBuffers(
        LogicalDevice, {CommandPool, vk::CommandBufferLevel::ePrimary, Swapchain.getImageCount()});
    spdlog::info("tRenderer: Created command buffers");
}

void tRenderer::createCommandPool()
{
    spdlog::info("tRenderer: Creating command pool...");
    vk::CommandPoolCreateInfo cpci({vk::CommandPoolCreateFlagBits::eResetCommandBuffer}, Device.getQueueFamily());
    CommandPool = LogicalDevice.createCommandPool(cpci);
    spdlog::info("tRenderer: Command pool created");
}

std::pair<vk::Result, uint32_t> tRenderer::acquireNextImage()
{
    ZoneScopedN("tRenderer: acquireNextImage()");
    spdlog::trace("tRenderer: Acquiring next image...");
    uint32_t ixImage{0};
    vk::Result result;
    try
    {
        std::tie(result, ixImage) =
            Swapchain.getSwapchain().acquireNextImage(UINT64_MAX, *ImageAvailable[IxCurrentFrame], nullptr);
    }
    catch (const vk::SystemError &err)
    {
        result = static_cast<vk::Result>(err.code().value());
    }
    spdlog::trace("tRenderer: Acquired image with index {}", ixImage);
    return {result, ixImage};
}

std::pair<vk::Result, uint32_t> tRenderer::synchronizeFrame()
{
    ZoneScopedN("tRenderer: synchronizeFrame()");
    spdlog::trace("tRenderer: Synchronizing frame with index {}...", IxCurrentFrame);
    const auto frameValue = FrameTimelineValues[IxCurrentFrame];
    if (frameValue > 0)
    {
        waitTimelineValue(frameValue);
    }

    const auto acquireResult = acquireNextImage();
    const auto result = acquireResult.first;
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        spdlog::warn("tRenderer: acquireNextImage returned {}", vk::to_string(result));
        recreateSwapchain(Swapchain.getExtent());
        return acquireResult;
    }

    const auto ixImage = acquireResult.second;
    const auto imageValue = ImageTimelineValues[ixImage];
    if (imageValue > frameValue)
    {
        waitTimelineValue(imageValue);
    }
    spdlog::trace("tRenderer: Synchronized frame with index {} and image with index {}", IxCurrentFrame, ixImage);
    return acquireResult;
}

void tRenderer::submit(const uint32_t ixImage)
{
    ZoneScopedN("tRenderer: submit()");
    spdlog::trace("tRenderer: Starting submit of command buffer at image index {}", ixImage);
    const uint64_t signalValue = ++LastTimelineValue;
    vk::SemaphoreSubmitInfo wsi(
        ImageAvailable[IxCurrentFrame], 0, vk::PipelineStageFlagBits2::eColorAttachmentOutput, 0);
    vk::SemaphoreSubmitInfo renderSignal(RenderFinished[ixImage], 0, vk::PipelineStageFlagBits2::eAllCommands, 0);
    vk::SemaphoreSubmitInfo timelineSignal(FrameTimeline, signalValue, vk::PipelineStageFlagBits2::eAllCommands, 0);

    vk::CommandBufferSubmitInfo simCbsi(SimCommandBuffers[ixImage]);
    vk::CommandBufferSubmitInfo cbsi(CommandBuffers[ixImage]);
    vk::CommandBufferSubmitInfo guiCbsi(GuiCommandBuffers[ixImage]);
    std::array bufferInfos{simCbsi, cbsi, guiCbsi};

    std::array waitInfos{wsi};
    std::array signalInfos{renderSignal, timelineSignal};
    vk::SubmitInfo2 si{};
    si.setWaitSemaphoreInfos(waitInfos).setCommandBufferInfos(bufferInfos).setSignalSemaphoreInfos(signalInfos);
    Queue.submit2(si);

    FrameTimelineValues[IxCurrentFrame] = signalValue;
    ImageTimelineValues[ixImage] = signalValue;
    spdlog::trace("tRenderer: Ending submit of command buffer at image index {}", ixImage);
}

void tRenderer::present(const uint32_t ixImage)
{
    ZoneScopedN("tRenderer: present()");
    spdlog::trace("tRenderer: Presenting image at image index {}...", ixImage);
    vk::PresentInfoKHR pi{};
    pi.swapchainCount = 1;
    pi.pSwapchains = &*Swapchain.getSwapchain();
    pi.pImageIndices = &ixImage;
    pi.waitSemaphoreCount = 1;
    pi.pWaitSemaphores = &*RenderFinished[ixImage];

    vk::Result result;
    try
    {
        result = Queue.presentKHR(pi);
    }
    catch (const vk::SystemError &err)
    {
        result = static_cast<vk::Result>(err.code().value());
    }
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        spdlog::warn("tRenderer: Present result is out of date or suboptimal");
        recreateSwapchain(Swapchain.getExtent());
    }
    spdlog::trace("tRenderer: Ending present of command buffer at image index {}", ixImage);
}

void tRenderer::recordReusableCommandBuffers()
{
    if (TracyContext != nullptr)
        return;

    const auto imageCount = Swapchain.getImageCount();
    for (uint32_t i = 0; i < imageCount; ++i)
    {
        recordGraphicsCommandBuffer(i);
    }
}

void tRenderer::recordPerFrameCommandBuffers(const uint32_t ixImage)
{
    ZoneScopedN("tRenderer: recordPerFrameCommandBuffers()");
    spdlog::trace("tRenderer: Recording per frame buffers for image {}...", ixImage);
    const auto &simBuffer = SimCommandBuffers[ixImage];
    simBuffer.reset();
    simBuffer.begin({});
    // Tracy GPU collection needs a recording command buffer outside a render pass; simBuffer is always recorded.
    if (TracyContext != nullptr)
    {
        TracyVkCollect(TracyContext, *simBuffer);
    }
    {
        TracyVkNamedZone(TracyContext, tracySimZone, *simBuffer, "Sim Command Buffer", true);
        Sim.recordComputePass(simBuffer, ixImage);
    }
    simBuffer.end();

    (this->*RecordGraphicsPerFrame)(ixImage);

    const auto &guiBuffer = GuiCommandBuffers[ixImage];
    guiBuffer.reset();
    guiBuffer.begin({});
    {
        TracyVkNamedZone(TracyContext, tracyGuiZone, *guiBuffer, "Gui Command Buffer", true);
        Gui.recordGuiPass(
            guiBuffer, Swapchain.getExtent(), Swapchain.getImage(ixImage), Swapchain.getImageView(ixImage));
    }
    guiBuffer.end();
    spdlog::trace("tRenderer: Recorded per frame buffers for image {}", ixImage);
}

void tRenderer::recordGraphicsPass(const vk::raii::CommandBuffer &buffer,
                                   const vk::Image &image,
                                   const vk::raii::ImageView &imageView)
{
    ZoneScopedN("tRenderer: recordGraphicsPass");
    spdlog::trace("tRenderer: Recording graphics pass...");
    TracyVkNamedZone(TracyContext, tracyGraphicsPassZone, *buffer, "Graphics Pass", true);
    vk::ClearValue clearColor{std::array<float, 4>{0.02f, 0.02f, 0.02f, 1.0f}};
    vk::ClearValue clearDepth{vk::ClearDepthStencilValue{1.0f, 0}};
    std::array<vk::ClearValue, 2> clears{clearColor, clearDepth};

    vk::RenderingAttachmentInfo colorAttachment{};
    colorAttachment.imageView = imageView;
    colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.clearValue = vk::ClearValue{vk::ClearColorValue(std::array<float, 4>{0, 0, 0, 1})};

    vk::RenderingAttachmentInfo depthAttachment{};
    depthAttachment.imageView = Swapchain.getDepthImageView();
    depthAttachment.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.clearValue = vk::ClearValue{vk::ClearDepthStencilValue{1.0f, 0}};

    vk::RenderingInfo ri{};
    ri.renderArea = vk::Rect2D{vk::Offset2D{}, Swapchain.getExtent()};
    ri.layerCount = 1;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments = &colorAttachment;
    ri.pDepthAttachment = &depthAttachment;

    vk::ImageMemoryBarrier2 preBarrier{};
    preBarrier.dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;
    preBarrier.dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite;
    preBarrier.oldLayout = vk::ImageLayout::ePresentSrcKHR;
    preBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
    preBarrier.image = image;
    preBarrier.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

    buffer.pipelineBarrier2({{}, {}, {}, preBarrier});
    buffer.beginRendering(ri);
    buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *GraphicsPipeline);

    const ParticlePushConstants particlePc{
        LogicalDevice.getBufferAddress(vk::BufferDeviceAddressInfo{*Sim.getParticleBuffer()}),
        NUM_PARTICLES,
        0u,
        0u,
        0u};
    const vk::PushConstantsInfo pushConstantsInfo{*GraphicsPipelineLayout,
                                                  vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT,
                                                  0,
                                                  sizeof(particlePc),
                                                  &particlePc};
    buffer.pushConstants2(pushConstantsInfo);
    buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, *GraphicsPipelineLayout, 1, *Camera.getDescriptorSet(), {});
    buffer.drawMeshTasksEXT(1, 1, 1);
    buffer.endRendering();
    spdlog::trace("tRenderer: Recorded graphics pass");
}

void tRenderer::recordGraphicsCommandBuffer(const uint32_t ixImage)
{
    spdlog::trace("tRenderer: Starting recording of command buffer at image index {}", ixImage);
    const auto &commandBuffer = CommandBuffers[ixImage];
    const auto &image = Swapchain.getImage(ixImage);
    const auto &view = Swapchain.getImageView(ixImage);
    commandBuffer.reset();
    commandBuffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eSimultaneousUse});
    {
        TracyVkNamedZone(TracyContext, tracyGraphicsZone, *commandBuffer, "Graphics Command Buffer", true);
        commandBuffer.pipelineBarrier2(vk::DependencyInfo{}.setMemoryBarriers(ComputeToGraphicsBarrier));
        recordGraphicsPass(commandBuffer, image, view);
    }

    commandBuffer.end();
    spdlog::trace("tRenderer: Ended recording of command buffer at image index {}", ixImage);
}

void tRenderer::waitTimelineValue(const uint64_t value)
{
    if (value == 0)
    {
        return;
    }

    const vk::Semaphore semaphores[] = {*FrameTimeline};
    const uint64_t values[] = {value};
    vk::SemaphoreWaitInfo waitInfo{{}, 1, semaphores, values};
    const auto result = LogicalDevice.waitSemaphores(waitInfo, UINT64_MAX);
    if (result != vk::Result::eSuccess)
    {
        spdlog::warn("tRenderer: waitSemaphores returned {}", vk::to_string(result));
    }
}
