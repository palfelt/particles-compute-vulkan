#include "engine/tGui.h"

#include <cstdio>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <tracy/Tracy.hpp>

#include "engine/tCamera.h"
#include "engine/tSwapchain.h"
#include "engine/tVulkanDevice.h"

tGui::tGui(tCamera &camera,
           const tVulkanDevice &device,
           const tSwapchain &swapchain,
           const vk::raii::Instance &instance,
           GLFWwindow &window)
    : Camera(camera), Window(window)
{
    spdlog::info("tGui: Initializing...");
    initImGui(device, instance, swapchain);
    spdlog::info("tGui: Initialized");
}

void tGui::update()
{
    spdlog::trace("tGui: Updating...");
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Debug");
    if (ImGui::BeginTabBar("DebugTabs"))
    {
        if (ImGui::BeginTabItem("Stats"))
        {
            updateFPSCounter();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
    handleCameraUserInputs();
    spdlog::trace("tGui: Updated");
}

void tGui::updateFPSCounter()
{
    ImGui::Text("FPS: %.1f", Io->Framerate);
}

void tGui::recordGuiPass(const vk::raii::CommandBuffer &commandBuffer,
                         const vk::Extent2D &extent,
                         const vk::Image &image,
                         const vk::ImageView &imageView) const
{
    ZoneScopedN("tGui: recordGuiPass()");
    vk::RenderingAttachmentInfo colorAttachment{};
    colorAttachment.imageView = imageView;
    colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;

    vk::RenderingInfo ri{};
    ri.renderArea = vk::Rect2D{vk::Offset2D{0, 0}, extent};
    ri.layerCount = 1;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments = &colorAttachment;
    ri.pDepthAttachment = nullptr;

    commandBuffer.beginRendering(ri);
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandBuffer);
    commandBuffer.endRendering();

    vk::ImageMemoryBarrier2 postRenderBarrier{vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                                              vk::AccessFlagBits2::eColorAttachmentWrite,
                                              vk::PipelineStageFlagBits2::eBottomOfPipe,
                                              vk::AccessFlagBits2::eNone,
                                              vk::ImageLayout::eColorAttachmentOptimal,
                                              vk::ImageLayout::ePresentSrcKHR,
                                              0,
                                              0,
                                              image,
                                              {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};
    commandBuffer.pipelineBarrier2(vk::DependencyInfo({}, {}, {}, postRenderBarrier));
}

void tGui::handleCameraUserInputs()
{
    ZoneScopedN("tGui: handleCameraUserInputs()");
    spdlog::trace("tGui: Handling camera user inputs...");
    const auto deltaTime = 1.f / Io->Framerate;
    handleCameraKeyboard(deltaTime);
    handleCameraMouse();
    spdlog::trace("tGui: Handled camera user inputs");
}

void tGui::handleCameraKeyboard(const float deltaTime)
{
    glm::vec3 moveDir{0.0f};

    const auto window = &Window;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        moveDir.z += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        moveDir.z -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        moveDir.x -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        moveDir.x += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        moveDir.y += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        moveDir.y -= 1.0f;

    if (glm::length(moveDir) > 0.0f)
    {
        moveDir = glm::normalize(moveDir);
        Camera.moveLocal(moveDir, MoveSpeed * deltaTime);
    }
}

void tGui::handleCameraMouse()
{
    const auto window = &Window;

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        IsFirstMouse = true;
        return;
    }

    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);

    if (IsFirstMouse)
    {
        LastMousePosX = xPos;
        LastMousePosY = yPos;
        IsFirstMouse = false;
        return;
    }

    const double xoffset = xPos - LastMousePosX;
    const double yoffset = yPos - LastMousePosY;
    LastMousePosX = xPos;
    LastMousePosY = yPos;

    const float deltaYaw = static_cast<float>(-xoffset) * MouseSensitivity;
    const float deltaPitch = static_cast<float>(-yoffset) * MouseSensitivity;
    Camera.addYawPitch(deltaYaw, deltaPitch);
}

void tGui::initImGui(const tVulkanDevice &device, const vk::raii::Instance &instance, const tSwapchain &swapchain)
{
    spdlog::info("tGui: Initializing ImGui...");
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    Io = &io;
    ImGui::StyleColorsDark();

    std::array<vk::DescriptorPoolSize, 11> poolSizes = {{{vk::DescriptorType::eSampler, 1000},
                                                         {vk::DescriptorType::eCombinedImageSampler, 1000},
                                                         {vk::DescriptorType::eSampledImage, 1000},
                                                         {vk::DescriptorType::eStorageImage, 1000},
                                                         {vk::DescriptorType::eUniformTexelBuffer, 1000},
                                                         {vk::DescriptorType::eStorageTexelBuffer, 1000},
                                                         {vk::DescriptorType::eUniformBuffer, 1000},
                                                         {vk::DescriptorType::eStorageBuffer, 1000},
                                                         {vk::DescriptorType::eUniformBufferDynamic, 1000},
                                                         {vk::DescriptorType::eStorageBufferDynamic, 1000},
                                                         {vk::DescriptorType::eInputAttachment, 1000}}};
    vk::DescriptorPoolCreateInfo dpci(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                                      1000 * static_cast<uint32_t>(poolSizes.size()),
                                      static_cast<uint32_t>(poolSizes.size()),
                                      poolSizes.data());
    ImGuiPool = vk::raii::DescriptorPool(device.getLogicalDevice(), dpci);

    vk::PipelineRenderingCreateInfo prci({}, swapchain.getColorFormat(), swapchain.getDepthFormat(), {});

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = *instance;
    initInfo.PhysicalDevice = *device.getPhysicalDevice();
    initInfo.Device = *device.getLogicalDevice();
    initInfo.QueueFamily = device.getQueueFamily();
    initInfo.Queue = *device.getQueue();
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = *ImGuiPool;
    initInfo.MinImageCount = swapchain.getMinImageCount();
    initInfo.ImageCount = swapchain.getImageCount();
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = prci;
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplGlfw_InitForVulkan(&Window, true);
    ImGui_ImplVulkan_Init(&initInfo);
    spdlog::info("tGui: Initialized ImGui");
}
