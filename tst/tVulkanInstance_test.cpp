#include <gtest/gtest.h>

#include "engine/tVulkanInstance.h"

TEST(tVulkanInstanceTest, VulkanInstanceInit)
{
    EXPECT_NO_THROW((tVulkanInstance{true}));
}