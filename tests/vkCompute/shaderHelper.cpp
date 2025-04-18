#include "shaderHelper.hpp"

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

VkShaderModule createShaderModule(vulkan& vk, const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(vk.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

void createBuffer(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
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

buffer createComputeBuffer(vulkan& vk, uint64_t size) {
    buffer buf;
    buf.size = size;
    createBuffer(vk.device, buf.size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vk.physicalDevice, buf.buffer, buf.memory);
    return buf;
}

void copyToBuffer(vulkan& vk, buffer& buf, void* data) {
    void* mappedData;
    vkMapMemory(vk.device, buf.memory, 0, buf.size, 0, &mappedData);
    memcpy(mappedData, data, (size_t) buf.size);
    vkUnmapMemory(vk.device, buf.memory);
}

void copyFromBuffer(vulkan& vk, buffer& buf, void* data) {
    void* mappedData;
    vkMapMemory(vk.device, buf.memory, 0, buf.size, 0, &mappedData);
    memcpy(data, mappedData, (size_t) buf.size);
    vkUnmapMemory(vk.device, buf.memory);
}

kernel buildKernal(vulkan& vk, VkShaderModule& shaderModule, std::vector<buffer>& buffers, size_t nThreads) {
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
    vkCreateDescriptorSetLayout(vk.device, &layoutInfo, nullptr, &kern.descriptorSetLayout);

    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &kern.descriptorSetLayout;
    // VkPipelineLayout pipelineLayout;
    vkCreatePipelineLayout(vk.device, &pipelineLayoutInfo, nullptr, &kern.pipelineLayout);

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
    vkCreateComputePipelines(vk.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &kern.pipeline);

    // Create descriptor pool
    VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2};
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    // VkDescriptorPool descriptorPool;
    vkCreateDescriptorPool(vk.device, &poolInfo, nullptr, &kern.descriptorPool);

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = kern.descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &kern.descriptorSetLayout;
    // VkDescriptorSet descriptorSet;
    vkAllocateDescriptorSets(vk.device, &allocInfo, &kern.descriptorSet);

    std::vector<VkWriteDescriptorSet> writes(buffers.size());
    std::vector<VkDescriptorBufferInfo> infos(buffers.size());

    for (size_t i=0; i<buffers.size(); ++i) {
        infos[i] = {buffers[i].buffer, 0, buffers[i].size};
        writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, kern.descriptorSet, static_cast<uint32_t>(i), 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &infos[i]};
    }
    vkUpdateDescriptorSets(vk.device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    // Record command buffer
    VkCommandBufferAllocateInfo cmdAllocInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cmdAllocInfo.commandPool = vk.commandPool;
    cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAllocInfo.commandBufferCount = 1;
    // VkCommandBuffer cmdBuf;
    vkAllocateCommandBuffers(vk.device, &cmdAllocInfo, &kern.cmdBuf);

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(kern.cmdBuf, &beginInfo);
    vkCmdBindPipeline(kern.cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, kern.pipeline);
    vkCmdBindDescriptorSets(kern.cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, kern.pipelineLayout, 0, 1, &kern.descriptorSet, 0, nullptr);
    vkCmdDispatch(kern.cmdBuf, nThreads, 1, 1);
    vkEndCommandBuffer(kern.cmdBuf);

    return kern;
}

void executeKernel(vulkan& vk, kernel& kern) {
    VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &kern.cmdBuf;

    vkQueueSubmit(vk.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vk.graphicsQueue);
}

// void destroyBuffer(vulkan& vk, buffer& buf) {
//     vkDestroyBuffer(vk.device, buf.buffer, nullptr);
//     vkFreeMemory(vk.device, buf.memory, nullptr);
// }
// void destroyShaderModule(vulkan& vk, VkShaderModule shaderModule) {
//     vkDestroyShaderModule(vk.device, shaderModule, nullptr);
// }
// void destroyPipeline(vulkan& vk, VkPipeline pipeline) {
//     vkDestroyPipeline(vk.device, pipeline, nullptr);
// }
// void destroyPipelineLayout(vulkan& vk, VkPipelineLayout pipelineLayout) {
//     vkDestroyPipelineLayout(vk.device, pipelineLayout, nullptr);
// }
// void destroyDescriptorSetLayout(vulkan& vk, VkDescriptorSetLayout descriptorSetLayout) {
//     vkDestroyDescriptorSetLayout(vk.device, descriptorSetLayout, nullptr);
// }
// void destroyCommandPool(vulkan& vk, VkCommandPool commandPool) {
//     vkDestroyCommandPool(vk.device, commandPool, nullptr);
// }
// void destroyDescriptorPool(vulkan& vk, VkDescriptorPool descriptorPool) {
//     vkDestroyDescriptorPool(vk.device, descriptorPool, nullptr);
// }
// void destroyCommandBuffer(vulkan& vk, VkCommandBuffer commandBuffer) {
//     vkFreeCommandBuffers(vk.device, vk.commandPool, 1, &commandBuffer);
// }