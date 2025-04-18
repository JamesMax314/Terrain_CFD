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

struct vulkan {
    bool enableValidationLayers;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkSurfaceKHR surface;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkCommandPool commandPool;
};

void setDebugMode(vulkan& vk, bool enable);
void createInstance(vulkan& vk, std::string appName, std::string engineName);
void setupDebugMessenger(vulkan& vk);
void createSurface(vulkan& vk, GLFWwindow* window);
void pickPhysicalDevice(vulkan& vk);
void createLogicalDevice(vulkan& vk);
void createCommandPool(vulkan& vk);

