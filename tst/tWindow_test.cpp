#include <gtest/gtest.h>

#include "engine/tWindow.h"

TEST(tWindowTest, WindowInit)
{
    EXPECT_NO_THROW({ tWindow(1, 1, "name"); });
}