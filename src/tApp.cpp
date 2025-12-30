#include "tApp.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <tracy/Tracy.hpp>
#include <vulkan/vulkan.h>

#include "engine/tCamera.h"
#include "engine/tGui.h"
#include "engine/tRenderer.h"
#include "sim/tSim.h"

tApp::tApp(const bool enableValidation)
    : ValidationEnabled(enableValidation), Instance(enableValidation), Window(WindowWidth, WindowHeight, Name)
{
    spdlog::info("tApp: Initializing...");
    Window.createWindowSurface(Instance.getInstance());
    Device.init(Instance.getInstance(), Window.getSurface(), ValidationEnabled);
    Swapchain.init(Instance.getInstance(), Device, Window.getSurface(), Window.getExtent());
    Sim = std::make_unique<tSim>(Device, Swapchain.getImageCount());
    Camera = std::make_unique<tCamera>(Device, Swapchain.getExtent());
    Gui = std::make_unique<tGui>(*Camera, Device, Swapchain, Instance.getInstance(), Window.getWindow());
    Renderer = std::make_unique<tRenderer>(*Camera, *Gui, Device, Swapchain, *Sim);
    spdlog::info("tApp: Initialized");
}

tApp::~tApp()
{
    Device.getLogicalDevice().waitIdle();
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    spdlog::info("tApp: Destroyed");
}

void tApp::run()
{
    spdlog::info("tApp: Running until application should close");
    while (!Window.shouldClose())
    {
        loop();
    }
}

float tApp::beginFrame()
{
    glfwPollEvents();
    const auto deltaTime = Timer.tick();
    ElapsedTime += deltaTime;
    return deltaTime;
}

void tApp::handleResize()
{
    if (!Window.wasResized())
    {
        return;
    }

    spdlog::info("tApp: Window was resized");
    auto extent = Window.getExtent();
    while (extent.width == 0 || extent.height == 0)
    {
        extent = Window.getExtent();
        glfwWaitEvents();
    }

    Camera->onResize(extent);
    Renderer->recreateSwapchain(extent);
    ImGui_ImplVulkan_SetMinImageCount(Swapchain.getMinImageCount());
    Window.resetResizedFlag();
}

void tApp::updateSimulation(float deltaTime)
{
    const tPhysics::tParams physicsParams{deltaTime};
    Sim->updateParams(physicsParams);
}

void tApp::updateGui()
{
    Gui->update();
}

void tApp::renderFrame()
{
    Camera->updateViewData();
    Renderer->drawFrame();
    Sim->swapParticleBuffers();
    FrameMark;
}

void tApp::loop()
{
    ZoneScopedN("tApp: loop()");
    const auto deltaTime = beginFrame();

    handleResize();
    updateSimulation(deltaTime);
    updateGui();
    renderFrame();
}
