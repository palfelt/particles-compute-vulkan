#pragma once

#include <optional>
#include <vector>

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

class tVulkanInstance
{
  public:
    explicit tVulkanInstance(bool enableValidation);
    ~tVulkanInstance() { spdlog::info("tVulkanInstance: Destroyed"); }

    const vk::raii::Instance &getInstance() const { return Instance; }
    const vk::detail::DispatchLoaderDynamic &getLoader() const { return Loader; }

  private:
    vk::ApplicationInfo generateAppInfo();
    vk::InstanceCreateInfo generateInstanceCreateInfo();
    bool checkValidationLayerSupport();
    std::vector<const char *> getRequiredExtensions(bool enableValidation);

    vk::raii::Context Context;
    vk::raii::Instance Instance{nullptr};
    vk::detail::DispatchLoaderDynamic Loader;
    std::optional<vk::raii::DebugUtilsMessengerEXT> DebugMessenger;
};
