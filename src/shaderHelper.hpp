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

int get_comp_queue(Init& init, ComputeHandler& handler);
int create_command_pool(Init& init, ComputeHandler& handler);

buffer create_compute_buffer(Init& init, VkDeviceSize size);

void copy_to_buffer(Init& init, buffer& buf, void* data);
void copy_from_buffer(Init& init, buffer& buf, void* data);

kernel build_kernal(Init& init, ComputeHandler& handler, VkShaderModule& shaderModule, std::vector<buffer>& buffers, size_t nThreads);
void updateDescriptorSetForPass(Init& init, std::vector<buffer>& buffers, VkDescriptorSet descriptorSet);

void execute_kernel(Init& init, ComputeHandler& handler, kernel& kern);

void cleanup(Init& init, std::vector<buffer>& buffers);
void cleanup(Init& init, ComputeHandler& handler);
void cleanup(Init& init, kernel& kern);
