#pragma once

#include "vkHelper.hpp"

struct buffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    uint64_t size;
};

struct ComputeHandler {
    VkCommandBuffer commandBuffer;
    VkCommandPool commandPool;
    VkQueue queue;
    VkFence fence;
    VkSemaphore semaphore;
    VkSubmitInfo submitInfo;
};

struct kernel {
    VkPipeline pipeline;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet descriptorSet;
    VkCommandBuffer cmdBuf;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
};

struct texture {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    VkImage image;
    VkDeviceMemory memory;
    VkImageView imageView;
    VkSampler sampler;
};

int get_comp_queue(Init& init, ComputeHandler& handler);
int create_command_pool(Init& init, ComputeHandler& handler);

buffer create_compute_buffer(Init& init, VkDeviceSize size);
int create_command_buffers(Init& init, RenderData& data, std::vector<texture>& textures);

void copy_to_buffer(Init& init, buffer& buf, void* data);
void copy_from_buffer(Init& init, buffer& buf, void* data);

kernel build_kernal(Init& init, ComputeHandler& handler, VkShaderModule& shaderModule, std::vector<buffer>& buffers, size_t nThreads);
void updateDescriptorSetForPass(Init& init, std::vector<buffer>& buffers, VkDescriptorSet descriptorSet);

void execute_kernel(Init& init, ComputeHandler& handler, kernel& kern);

void createImage(Init& init, texture& texture);
void createImageView(Init& init, texture& tex);
void createSampler(Init& init, texture& tex);
void createTextureMemory(Init& init, texture& tex);
void create3DTexture(Init& init, texture& tex);

void cleanup(Init& init, std::vector<buffer>& buffers);
void cleanup(Init& init, ComputeHandler& handler);
void cleanup(Init& init, kernel& kern);
void cleanup(Init& init, texture& tex);
