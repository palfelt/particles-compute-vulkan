#include <gtest/gtest.h>

#include "tApp.h"

TEST(tAppTest, AppInit)
{
    EXPECT_NO_THROW((tApp{true}));
}