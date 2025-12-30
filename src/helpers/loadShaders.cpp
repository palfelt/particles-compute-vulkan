#include "helpers/loadShaders.h"

#include <fstream>

#ifndef SHADER_DIR
#define SHADER_DIR "./shaders"
#warning "SHADER_DIR not defined — using default ./shaders"
#endif

vk::raii::ShaderModule loadShaderModule(const vk::raii::Device &device, const std::string &fileName)
{
    const auto path = std::string(SHADER_DIR) + fileName;
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open SPIR-V: " + path);
    const size_t size = static_cast<size_t>(file.tellg());
    if (size == 0 || (size % 4) != 0)
        throw std::runtime_error("Bad SPIR-V size: " + path);
    std::vector<char> code(size);
    file.seekg(0);
    file.read(code.data(), size);

    vk::ShaderModuleCreateInfo ci{};
    ci.codeSize = code.size();
    ci.pCode = reinterpret_cast<const uint32_t *>(code.data());
    return vk::raii::ShaderModule(device, ci);
}