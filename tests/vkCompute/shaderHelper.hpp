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

std::vector<char> readFile(const std::string& filename);
VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);
void createBuffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
    VkPhysicalDevice physicalDevice, VkBuffer& buffer, VkDeviceMemory& memory);

