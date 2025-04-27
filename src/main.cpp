#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vkHelper.hpp"
#include "shaderHelper.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const uint local_work_size = 32;

// std::vector<Vertex> vertices = {
//     {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
//     {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
//     {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
//     {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}
// };

// std::vector<uint32_t> indices = {
//     0, 1, 2, 2, 3, 0
// };

std::vector<float> init_velocities(size_t gridsize, float vx, float vy, float vz) {
    std::vector<float> velocities(gridsize * gridsize * gridsize * 3);
    for (size_t i = 0; i < velocities.size(); i += 3) {
        velocities[i] = vx;
        velocities[i + 1] = vy;
        velocities[i + 2] = vz;
    }
    return velocities;
}

std::vector<float> init_scalars(size_t gridsize, float base_val) {
    std::vector<float> scalars(gridsize * gridsize * gridsize);
    for (size_t i = 0; i < scalars.size(); i += 1) {
        scalars[i] = base_val;
    }
    return scalars;
}

std::vector<float> init_vels(size_t gridsize, float base_val) {
    std::vector<float> scalars((gridsize+1) * gridsize * gridsize);
    for (size_t i = 0; i < scalars.size(); i += 1) {
        scalars[i] = base_val;
    }
    return scalars;
}

std::vector<float> init_boundaries(size_t gridsize) {
    std::vector<float> scalars(gridsize * gridsize * gridsize);
    for (size_t i = 0; i < scalars.size(); i += 1) {
        uint x = i % gridsize;
        uint y = (i / gridsize) % gridsize;
        uint z = i / (gridsize * gridsize);
        if (x % (gridsize-1) == 0 || y % (gridsize-1) == 0 || z % (gridsize-1) == 0) {
            scalars[i] = 0.0;
        } else {
            scalars[i] = 1.0;
        }
    }
    return scalars;
}

void print_vector(const std::vector<float>& vec) {
    for (const auto& val : vec) {
        std::cout << val << " ";
    }
    std::cout << "\n";
    std::cout << "\n";
}

void print_matrix(const std::vector<float>& mat, size_t rows) {
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < rows; ++j) {
            for (size_t k = 0; k < rows; ++k) {
                std::cout << mat[i * rows*rows + j*rows + k] << " ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }
}

kernel build_compute_kernal(Init& init, ComputeHandler& handler, VkShaderModule& shaderModule, std::vector<buffer>& buffers, std::vector<texture>& textures, size_t nThreads) {
    kernel kern;
    
    // Descriptor set bindings
    // VkDescriptorSetLayoutBinding bindings[buffers.size()]{};
    std::vector<VkDescriptorSetLayoutBinding> bindings(buffers.size() + textures.size());

    for (size_t i = 0; i < buffers.size(); ++i) {
        bindings[i].binding = static_cast<uint32_t>(i);
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[i].pImmutableSamplers = nullptr;
    }

    for (size_t i = 0; i < textures.size(); ++i) {
        bindings[buffers.size() + i].binding = static_cast<uint32_t>(buffers.size() + i);
        bindings[buffers.size() + i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[buffers.size() + i].descriptorCount = 1;
        bindings[buffers.size() + i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[buffers.size() + i].pImmutableSamplers = nullptr;
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
    VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(buffers.size())},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, static_cast<uint32_t>(textures.size())}};
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
    poolInfo.pPoolSizes = poolSizes;
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

    std::vector<VkWriteDescriptorSet> writes(buffers.size() + textures.size());
    std::vector<VkDescriptorBufferInfo> infos(buffers.size());
    std::vector<VkDescriptorImageInfo> texInfos(textures.size());

    for (size_t i=0; i<buffers.size(); ++i) {
        infos[i] = {buffers[i].buffer, 0, buffers[i].size};
        writes[i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, kern.descriptorSet, static_cast<uint32_t>(i), 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr, &infos[i]};
    }

    for (size_t i = 0; i < textures.size(); ++i) {
        texInfos[i] = {textures[i].sampler, textures[i].imageView, VK_IMAGE_LAYOUT_GENERAL};
        writes[buffers.size() + i] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, kern.descriptorSet, static_cast<uint32_t>(buffers.size() + i), 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &texInfos[i], nullptr};
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

    // Transition image layouts
    for (size_t i = 0; i < textures.size(); ++i) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = textures[i].image; // your VkImage
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(
            kern.cmdBuf, // command buffer you're recording to
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );
    }

    vkCmdDispatch(kern.cmdBuf, nThreads, 1, 1);
    vkEndCommandBuffer(kern.cmdBuf);

    return kern;
}

int create_command_buffers(Init& init, RenderData& data, std::vector<texture>& textures) {
    data.command_buffers.resize(data.framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = data.command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)data.command_buffers.size();

    if (init.disp.allocateCommandBuffers(&allocInfo, data.command_buffers.data()) != VK_SUCCESS) {
        return -1; // failed to allocate command buffers;
    }

    for (size_t i = 0; i < data.command_buffers.size(); i++) {
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (init.disp.beginCommandBuffer(data.command_buffers[i], &begin_info) != VK_SUCCESS) {
            return -1; // failed to begin recording command buffer
        }

        VkRenderPassBeginInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = data.render_pass;
        render_pass_info.framebuffer = data.framebuffers[i];
        render_pass_info.renderArea.offset = { 0, 0 };
        render_pass_info.renderArea.extent = init.swapchain.extent;
        VkClearValue clearColor{ { {202.0f/255.0f, 226.0f/255.0f, 232.0f/255.0f, 1.0f} } };
        render_pass_info.clearValueCount = 1;
        render_pass_info.pClearValues = &clearColor;

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)init.swapchain.extent.width;
        viewport.height = (float)init.swapchain.extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = { 0, 0 };
        scissor.extent = init.swapchain.extent;

        init.disp.cmdSetViewport(data.command_buffers[i], 0, 1, &viewport);
        init.disp.cmdSetScissor(data.command_buffers[i], 0, 1, &scissor);

        for (size_t j = 0; j < textures.size(); ++j) {
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.image = textures[j].image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(
                data.command_buffers[i],
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        }

        init.disp.cmdBeginRenderPass(data.command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        init.disp.cmdBindPipeline(data.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, data.graphics_pipeline);

        vkCmdBindDescriptorSets(
            data.command_buffers[i],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            data.pipeline_layout,
            0,  // firstSet
            1, &data.descriptorSet,  // Use your descriptor set
            0, nullptr
        );        

        init.disp.cmdDraw(data.command_buffers[i], 4, 1, 0, 0);

        init.disp.cmdEndRenderPass(data.command_buffers[i]);

        if (init.disp.endCommandBuffer(data.command_buffers[i]) != VK_SUCCESS) {
            std::cout << "failed to record command buffer\n";
            return -1; // failed to record command buffer!
        }
    }
    return 0;
}

int create_graphics_pipeline(Init& init, RenderData& data, std::vector<texture>& textures) {
    auto vert_code = readFile(std::string(SHADER_DIR) + "/triangleVert.spv");
    auto frag_code = readFile(std::string(SHADER_DIR) + "/triangleFrag.spv");

    VkShaderModule vert_module = createShaderModule(init, vert_code);
    VkShaderModule frag_module = createShaderModule(init, frag_code);
    if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE) {
        std::cout << "failed to create shader module\n";
        return -1; // failed to create shader modules
    }

    VkPipelineShaderStageCreateInfo vert_stage_info = {};
    vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = vert_module;
    vert_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info = {};
    frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = frag_module;
    frag_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stages[] = { vert_stage_info, frag_stage_info };

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.vertexAttributeDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)init.swapchain.extent.width;
    viewport.height = (float)init.swapchain.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = init.swapchain.extent;

    VkPipelineViewportStateCreateInfo viewport_state = {};
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blending = {};
    color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &colorBlendAttachment;
    color_blending.blendConstants[0] = 0.0f;
    color_blending.blendConstants[1] = 0.0f;
    color_blending.blendConstants[2] = 0.0f;
    color_blending.blendConstants[3] = 0.0f;


    // Number of descriptors of each type you need
    uint32_t maxSets = 1;
    uint32_t maxDescriptors = textures.size(); // Number of textures, for example

    std::vector<VkDescriptorSetLayoutBinding> bindings(textures.size());

    for (size_t i = 0; i < textures.size(); ++i) {
        VkDescriptorSetLayoutBinding imageBinding{};
        imageBinding.binding = static_cast<uint32_t>(i);
        imageBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        imageBinding.descriptorCount = 1;
        imageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        imageBinding.pImmutableSamplers = nullptr;
        bindings[i] = imageBinding;
    }

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutCreateInfo.pBindings = bindings.data();

    vkCreateDescriptorSetLayout(init.device, &layoutCreateInfo, nullptr, &data.descriptorSetLayout);

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = maxDescriptors;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = maxSets;

    vkCreateDescriptorPool(init.device, &poolInfo, nullptr, &data.descriptorPool);

    // Allocate descriptor sets
    VkDescriptorSetAllocateInfo allocInfoDS = {};
    allocInfoDS.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfoDS.descriptorPool = data.descriptorPool;
    allocInfoDS.descriptorSetCount = 1;
    allocInfoDS.pSetLayouts = &data.descriptorSetLayout;

    vkAllocateDescriptorSets(init.device, &allocInfoDS, &data.descriptorSet);

    for (size_t i = 0; i < textures.size(); ++i) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageView = textures[i].imageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.sampler = textures[i].sampler;
    
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = data.descriptorSet;
        descriptorWrite.dstBinding = i;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;
    
        vkUpdateDescriptorSets(init.device, 1, &descriptorWrite, 0, nullptr);
    }

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 1;  // We have one descriptor set layout
    pipeline_layout_info.pSetLayouts = &data.descriptorSetLayout;
    pipeline_layout_info.pushConstantRangeCount = 0;

    if (init.disp.createPipelineLayout(&pipeline_layout_info, nullptr, &data.pipeline_layout) != VK_SUCCESS) {
        std::cout << "failed to create pipeline layout\n";
        return -1; // failed to create pipeline layout
    }

    std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

    VkPipelineDynamicStateCreateInfo dynamic_info = {};
    dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
    dynamic_info.pDynamicStates = dynamic_states.data();

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages;
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_info;
    pipeline_info.layout = data.pipeline_layout;
    pipeline_info.renderPass = data.render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

    if (init.disp.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &data.graphics_pipeline) != VK_SUCCESS) {
        std::cout << "failed to create pipline\n";
        return -1; // failed to create graphics pipeline
    }

    init.disp.destroyShaderModule(frag_module, nullptr);
    init.disp.destroyShaderModule(vert_module, nullptr);
    return 0;
}


kernel gaussSiedelKernel(Init& init, ComputeHandler& handler, VkShaderModule& shaderModule, std::vector<buffer>& buffers, size_t nThreads) {
    kernel kern;
    
    // Descriptor set bindings
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

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT; // or VERTEX/FRAGMENT
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(int);

    // Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &kern.descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
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
    VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(buffers.size())}};
    VkDescriptorPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
    poolInfo.pPoolSizes = poolSizes;
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

    int shouldRed = 1;
    vkCmdPushConstants(kern.cmdBuf, kern.pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &shouldRed);

    vkCmdBindPipeline(kern.cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, kern.pipeline);
    vkCmdBindDescriptorSets(kern.cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, kern.pipelineLayout, 0, 1, &kern.descriptorSet, 0, nullptr);

    vkCmdDispatch(kern.cmdBuf, nThreads, 1, 1);

    shouldRed = 0;
    vkCmdPushConstants(kern.cmdBuf, kern.pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(int), &shouldRed);

    vkCmdDispatch(kern.cmdBuf, nThreads, 1, 1);
    vkEndCommandBuffer(kern.cmdBuf);

    return kern;
}

int main() {
    Init init;
    RenderData render_data;
    ComputeHandler compute_handler;


    if (0 != device_initialization(init)) return -1;
    if (0 != create_swapchain(init)) return -1;
    if (0 != get_queues(init, render_data)) return -1;
    if (0 != create_render_pass(init, render_data)) return -1;

    if (0 != get_comp_queue(init, compute_handler)) return -1;
    if (0 != create_command_pool(init, compute_handler)) return -1;

    // Later will need a different render pass to draw standard geometry
    const int gridSize = 129;
    texture tex;
    tex.x = gridSize;
    tex.y = gridSize;
    tex.z = gridSize;
    create3DTexture(init, tex);
    std::vector<texture> textures = {tex};

    if (0 != create_graphics_pipeline(init, render_data, textures)) return -1;
    if (0 != create_framebuffers(init, render_data)) return -1;
    if (0 != create_command_pool(init, render_data)) return -1;
    if (0 != create_command_buffers(init, render_data, textures)) return -1;
    if (0 != create_sync_objects(init, render_data)) return -1;

    const int bufferSize = gridSize * gridSize * gridSize * sizeof(float);
    const int velBufferSize = (gridSize+1) * gridSize * gridSize * sizeof(float);
    const int boarderBufferSize = (gridSize+2) * (gridSize+2) * (gridSize+2) * sizeof(float);
    const int nThreads = (gridSize * gridSize * gridSize + local_work_size - 1) / local_work_size;

    buffer vx = create_compute_buffer(init, velBufferSize);
    buffer vy = create_compute_buffer(init, velBufferSize);
    buffer vz = create_compute_buffer(init, velBufferSize);
    buffer boundaries = create_compute_buffer(init, boarderBufferSize);

    std::vector<buffer> buffersGaussSiedel = {vx, vy, vz, boundaries};
    VkShaderModule shaderGaussSiedel = createShaderModule(init, readFile(std::string(SHADER_DIR) + "/gaussSiedel.spv"));
    kernel kernGaussSiedel = gaussSiedelKernel(init, compute_handler, shaderGaussSiedel, buffersGaussSiedel, nThreads);

    buffer density = create_compute_buffer(init, bufferSize);
    buffer pressure = create_compute_buffer(init, bufferSize);

    buffer vx2 = create_compute_buffer(init, velBufferSize);
    buffer vy2 = create_compute_buffer(init, velBufferSize);
    buffer vz2 = create_compute_buffer(init, velBufferSize);
    buffer density2 = create_compute_buffer(init, bufferSize);
    buffer pressure2 = create_compute_buffer(init, bufferSize);
    std::vector<buffer> buffers = {vx, vy, vz, density, pressure, vx2, vy2, vz2, density2, pressure2, boundaries};
    std::vector<buffer> buffers2 = {vx2, vy2, vz2, density2, pressure2, vx, vy, vz, density, pressure, boundaries};

    // std::vector<float> velocities = init_velocities(gridSize, 1.0f, 0.0f, 0.0f);
    std::vector<float> vxs = init_vels(gridSize, 0.0f);
    std::vector<float> vys = init_vels(gridSize, 0.0f);
    std::vector<float> vzs = init_vels(gridSize, 0.0f);
    std::vector<float> densities = init_scalars(gridSize, 0.0f);
    std::vector<float> boundariesVec = init_boundaries(gridSize+2);
    // densities[50] = 1.0f;
    vxs[gridSize*gridSize*1 + gridSize*1 + 0] = 10.0f;
    // vxs[gridSize*gridSize*64 + gridSize*64 + 10] = 100.0f;
    // vxs[gridSize*gridSize*64 + gridSize*64 + 11] = 100.0f;
    // vxs[gridSize*gridSize*64 + gridSize*64 + 12] = 100.0f;
    // vys[0] = 1.0f;
    // vys[50] = 1.0f;
    // vxs[10] = -1.0f;

    // copy_to_buffer(init, velocity, velocities.data());
    copy_to_buffer(init, vx, vxs.data());
    copy_to_buffer(init, vy, vys.data());
    copy_to_buffer(init, vz, vzs.data());
    copy_to_buffer(init, density, densities.data());
    copy_to_buffer(init, boundaries, boundariesVec.data());

    // print_vector(boundariesVec);

    // print_vector(densities);

    VkShaderModule shaderModule = createShaderModule(init, readFile(std::string(SHADER_DIR) + "/advect.spv"));
    // VkShaderModule shaderModule2 = createShaderModule(init, readFile(std::string(SHADER_DIR) + "/advect2.spv"));
    kernel kern = build_compute_kernal(init, compute_handler, shaderModule, buffers, textures, nThreads);
    kernel kern2 = build_compute_kernal(init, compute_handler, shaderModule, buffers2, textures, nThreads);

    // execute_kernel(init, compute_handler, kern);


    bool toggle = false;
    while (!glfwWindowShouldClose(init.window)) {
        glfwPollEvents();
        int res = draw_frame(init, render_data);
        if (res != 0) {
            std::cout << "failed to draw frame \n";
            return -1;
        }

        for (int i=0; i<5; i++)
        {
            execute_kernel(init, compute_handler, kernGaussSiedel);
        }
        
        if (toggle) {
            execute_kernel(init, compute_handler, kern2);
            // copy_from_buffer(init, density, densities.data());
        } else {
            execute_kernel(init, compute_handler, kern);
            // copy_from_buffer(init, density2, densities.data());
        }
        toggle = !toggle;

        // copy_to_buffer(init, density, densities.data());
        // kern = build_kernal(init, compute_handler, shaderModule, buffers, bufferSize);
        // print_vector(densities);
    }
    init.disp.deviceWaitIdle();

    // copy_from_buffer(init, vz, vzs.data());
    // print_vector(vzs);

    // copy_from_buffer(init, vy, vys.data());
    // print_vector(vys);

    // copy_from_buffer(init, vx, vxs.data());
    // print_vector(vxs);


    vkDestroyShaderModule(init.device.device, shaderModule, nullptr);
    cleanup(init, buffers);
    cleanup(init, buffersGaussSiedel);
    cleanup(init, tex);
    cleanup(init, kern);
    cleanup(init, kern2);
    cleanup(init, kernGaussSiedel);
    cleanup(init, compute_handler);
    cleanup(init, render_data);

    return 0;
}