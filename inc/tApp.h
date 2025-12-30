#pragma once

#include <spdlog/spdlog.h>

#include "engine/tSwapchain.h"
#include "engine/tVulkanDevice.h"
#include "engine/tVulkanInstance.h"
#include "engine/tWindow.h"
#include "tTimer.h"

class tCamera;
class tGui;
class tRenderer;
class tSim;

class tApp
{
  public:
    tApp(bool enableValidation = true);
    ~tApp();

    tApp(const tApp &) = delete;
    tApp &operator=(const tApp &) = delete;

    static constexpr int WindowWidth = 1700;
    static constexpr int WindowHeight = 900;
    static constexpr std::string_view Name = "vulkan-compute";

    void run();

  private:
    void loop();
    float beginFrame();
    void handleResize();
    void updateSimulation(float deltaTime);
    void updateGui();
    void renderFrame();

    // order matters w.r.t. destruction - last is destroyed first
    tVulkanInstance Instance;
    tVulkanDevice Device;
    tWindow Window;
    tSwapchain Swapchain;
    tTimer Timer;

    std::unique_ptr<tSim> Sim{nullptr};
    std::unique_ptr<tRenderer> Renderer{nullptr};
    std::unique_ptr<tCamera> Camera{nullptr};
    std::unique_ptr<tGui> Gui{nullptr};

    const bool ValidationEnabled;
    float ElapsedTime{0.f};
};
