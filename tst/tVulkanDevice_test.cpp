#include <gtest/gtest.h>

#include "engine/tVulkanDevice.h"
#include "engine/tVulkanInstance.h"
#include "engine/tWindow.h"

TEST(tVulkanDeviceTest, VulkanDeviceInit)
{
    const bool validatoinEnabled = false;
    tVulkanInstance instance(validatoinEnabled);
    tWindow window(1, 1, "test");
    window.createWindowSurface(instance.getInstance());

    tVulkanDevice device{};
    EXPECT_NO_THROW((device.init(instance.getInstance(), window.getSurface(), validatoinEnabled)));
}