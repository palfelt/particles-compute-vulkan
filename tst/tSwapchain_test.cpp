#include <gtest/gtest.h>

#include "engine/tSwapchain.h"
#include "engine/tVulkanDevice.h"
#include "engine/tVulkanInstance.h"
#include "engine/tWindow.h"

TEST(tSwapchainTest, SwapchainInit)
{
    const bool validatoinEnabled = false;
    tVulkanInstance instance(validatoinEnabled);
    tWindow window(1, 1, "test");
    window.createWindowSurface(instance.getInstance());

    tVulkanDevice device{};
    device.init(instance.getInstance(), window.getSurface(), validatoinEnabled);

    tSwapchain swapchain{};

    EXPECT_NO_THROW((swapchain.init(instance.getInstance(), device, window.getSurface(), window.getExtent())));
}