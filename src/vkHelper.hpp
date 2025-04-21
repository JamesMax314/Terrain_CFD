#pragma once

#include <stdio.h>

#include <memory>
#include <iostream>
#include <fstream>
#include <string>

#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>

#include <VkBootstrap.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

struct Init {
    GLFWwindow* window;
    vkb::Instance instance;
    vkb::InstanceDispatchTable inst_disp;
    VkSurfaceKHR surface;
    vkb::Device device;
    vkb::DispatchTable disp;
    vkb::Swapchain swapchain;
};

struct RenderData {
    VkQueue graphics_queue;
    VkQueue present_queue;

    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    std::vector<VkFramebuffer> framebuffers;

    VkRenderPass render_pass;
    VkPipelineLayout pipeline_layout;
    VkPipeline graphics_pipeline;

    VkCommandPool command_pool;
    std::vector<VkCommandBuffer> command_buffers;

    std::vector<VkSemaphore> available_semaphores;
    std::vector<VkSemaphore> finished_semaphore;
    std::vector<VkFence> in_flight_fences;
    std::vector<VkFence> image_in_flight;
    size_t current_frame = 0;
};

int device_initialization(Init& init);
int create_swapchain(Init& init);
int get_queues(Init& init, RenderData& data);
int create_render_pass(Init& init, RenderData& data);
int create_graphics_pipeline(Init& init, RenderData& data);
int create_framebuffers(Init& init, RenderData& data);
int create_command_pool(Init& init, RenderData& data);
int create_command_buffers(Init& init, RenderData& data);
int create_sync_objects(Init& init, RenderData& data);
int draw_frame(Init& init, RenderData& data);
void cleanup(Init& init, RenderData& data);

VkShaderModule createShaderModule(Init& init, const std::vector<char>& code);
std::vector<char> readFile(const std::string& filename);