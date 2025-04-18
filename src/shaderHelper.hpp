#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>

#include "vkHelper.hpp"

struct buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    uint64_t size;
};

struct kernel {
    VkPipeline pipeline;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;
    VkCommandBuffer cmdBuf;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
};

std::vector<char> readFile(const std::string& filename);
VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
VkShaderModule createShaderModule(vulkan& vk, const std::vector<char>& code);

void createBuffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
    VkPhysicalDevice physicalDevice, VkBuffer& buffer, VkDeviceMemory& memory);
buffer createComputeBuffer(vulkan& vk, VkDeviceSize size);
void copyToBuffer(vulkan& vk, buffer& buf, void* data);
void copyFromBuffer(vulkan& vk, buffer& buf, void* data);

kernel buildKernal(vulkan& vk, VkShaderModule& shaderModule, std::vector<buffer>& buffers, size_t nThreads);

void executeKernel(vulkan& vk, kernel& kern);