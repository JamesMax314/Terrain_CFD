#include "shaderHelper.hpp"

int get_comp_queue(Init& init, ComputeHandler& handler) {
    auto gq = init.device.get_queue(vkb::QueueType::graphics); // get compute queue; only one queue on some systems so use graphics
    if (!gq.has_value()) {
        std::cout << "failed to get graphics queue: " << gq.error().message() << "\n";
        return -1;
    }
    handler.queue = gq.value();
    return 0;
}

int create_command_pool(Init& init, ComputeHandler& handler) {
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = init.device.get_queue_index(vkb::QueueType::graphics).value();

    if (init.disp.createCommandPool(&pool_info, nullptr, &handler.commandPool) != VK_SUCCESS) {
        std::cout << "failed to create command pool\n";
        return -1; // failed to create command pool
    }
    return 0;
}

void create_buffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
    VkPhysicalDevice physicalDevice, VkBuffer& buffer, VkDeviceMemory& memory) {
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memRequirements.size;

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if (memRequirements.memoryTypeBits & (1 << i) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            allocInfo.memoryTypeIndex = i;
            break;
        }
    }

    vkAllocateMemory(device, &allocInfo, nullptr, &memory);
    vkBindBufferMemory(device, buffer, memory, 0);
}

buffer create_compute_buffer(Init& init, uint64_t size) {
    buffer buf;
    buf.size = size;
    create_buffer(init.device.device, buf.size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        init.device.physical_device, buf.buffer, buf.memory);
    return buf;
}

void copy_to_buffer(Init& init, buffer& buf, void* data) {
    void* mappedData;
    vkMapMemory(init.device.device, buf.memory, 0, buf.size, 0, &mappedData);
    memcpy(mappedData, data, (size_t) buf.size);
    vkUnmapMemory(init.device.device, buf.memory);
}

void copy_from_buffer(Init& init, buffer& buf, void* data) {
    void* mappedData;
    vkMapMemory(init.device.device, buf.memory, 0, buf.size, 0, &mappedData);
    memcpy(data, mappedData, (size_t) buf.size);
    vkUnmapMemory(init.device.device, buf.memory);
}

kernel build_kernal(Init& init, ComputeHandler& handler, VkShaderModule& shaderModule, std::vector<buffer>& buffers, size_t nThreads) {
    kernel kern;
    
    // Descripto set bindings
    // VkDescriptorSetLayoutBinding bindings[buffers.size()]{};
    std::vector<VkDescriptorSetLayoutBinding> bindings(buffers.size());

    for (size_t i = 0; i < buffers.size(); ++i) {
        bindings[i].binding = static_cast<uint32_t>(i);
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[i].pImmutableSamplers = nullptr;
    }

    // Descriptor Set Layout
    VkDescriptorSetLayoutCreateInfo layoutInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();
    // VkDescriptorSetLayout descriptorSetLayout;
    vkCreateDescriptorSetLayout(init.device.device, &layoutInfo, nullptr, &kern.descriptorSetLayout);

    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &kern.descriptorSetLayout;
    // VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(init.device.device, &pipelineLayoutInfo, nullptr, &kern.pipelineLayout);

    // Create compute pipeline
    // Shader stage
    VkPipelineShaderStageCreateInfo stageInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = shaderModule;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    pipelineInfo.stage = stageInfo;
    pipelineInfo.layout = kern.pipelineLayout;
    // VkPipeline pipeline;
    vkCreateComputePipelines(init.device.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &kern.pipeline);

    // Create descriptor pool
    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(buffers.size())};
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    // VkDescriptorPool descriptorPool;
    vkCreateDescriptorPool(init.device.device, &poolInfo, nullptr, &kern.descriptorPool);

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = kern.descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &kern.descriptorSetLayout;
    // VkDescriptorSet descriptorSet;
    vkAllocateDescriptorSets(init.device.device, &allocInfo, &kern.descriptorSet);

    std::vector<VkWriteDescriptorSet> writes(buffers.size());
    std::vector<VkDescriptorBufferInfo> infos(buffers.size());

    for (size_t i=0; i<buffers.size(); ++i) {
        infos[i] = {buffers[i].buffer, 0, buffers[i].size};
        writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, kern.descriptorSet, static_cast<uint32_t>(i), 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &infos[i]};
    }
    vkUpdateDescriptorSets(init.device.device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    // Record command buffer
    VkCommandBufferAllocateInfo cmdAllocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAllocInfo.commandPool = handler.commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    // VkCommandBuffer cmdBuf;
    vkAllocateCommandBuffers(init.device.device, &cmdAllocInfo, &kern.cmdBuf);

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(kern.cmdBuf, &beginInfo);
    vkCmdBindPipeline(kern.cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, kern.pipeline);
    vkCmdBindDescriptorSets(kern.cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, kern.pipelineLayout, 0, 1, &kern.descriptorSet, 0, nullptr);
    vkCmdDispatch(kern.cmdBuf, nThreads, 1, 1);
    vkEndCommandBuffer(kern.cmdBuf);

    return kern;
}

void execute_kernel(Init& init, ComputeHandler handler, kernel& kern) {
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &kern.cmdBuf;

    vkQueueSubmit(handler.queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(handler.queue);
}

void cleanup(Init& init, std::vector<buffer>& buffers) {
    for (auto& buf : buffers) {
        vkDestroyBuffer(init.device.device, buf.buffer, nullptr);
        vkFreeMemory(init.device.device, buf.memory, nullptr);
    }
}

void cleanup(Init& init, ComputeHandler& handler) {
    vkFreeCommandBuffers(init.device.device, handler.commandPool, 1, &handler.commandBuffer);
    vkDestroyCommandPool(init.device, handler.commandPool, nullptr);
}

void cleanup(Init& init, kernel& kern) {
    vkDestroyPipeline(init.device.device, kern.pipeline, nullptr);
    vkDestroyDescriptorPool(init.device.device, kern.descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(init.device.device, kern.descriptorSetLayout, nullptr);
    vkDestroyPipelineLayout(init.device.device, kern.pipelineLayout, nullptr);
}