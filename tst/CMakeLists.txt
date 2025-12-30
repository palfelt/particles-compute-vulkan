enable_testing()

add_executable(
  ${PROJECT_NAME}-test
  tApp_test.cpp
  tRenderer_test.cpp
  tSwapchain_test.cpp
  tVulkanDevice_test.cpp
  tVulkanInstance_test.cpp
  tWindow_test.cpp
)

target_link_libraries(
  ${PROJECT_NAME}-test
  PRIVATE
  app
  GTest::GTest
  GTest::Main
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}-test)
