#include <gtest/gtest.h>

#include "engine/tCamera.h"
#include "engine/tGui.h"
#include "engine/tRenderer.h"
#include "engine/tSwapchain.h"
#include "engine/tVulkanDevice.h"
#include "engine/tVulkanInstance.h"
#include "engine/tWindow.h"
#include "sim/tSim.h"

TEST(tRendererTest, RendererInit)
{
    const bool validatoinEnabled = false;
    tVulkanInstance instance(validatoinEnabled);
    tWindow window(1, 1, "test");
    window.createWindowSurface(instance.getInstance());
    tVulkanDevice device{};
    device.init(instance.getInstance(), window.getSurface(), validatoinEnabled);
    tSwapchain swapchain{};
    swapchain.init(instance.getInstance(), device, window.getSurface(), window.getExtent());
    tCamera camera{device, swapchain.getExtent()};
    tGui gui{camera, device, swapchain, instance.getInstance(), window.getWindow()};
    tSim sim{device, swapchain.getImageCount()};
    EXPECT_NO_THROW((tRenderer{camera, gui, device, swapchain, sim}));
}