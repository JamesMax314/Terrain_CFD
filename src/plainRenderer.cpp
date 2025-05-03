#include "plainRenderer.hpp"

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
    input_assembly.primitiveRestartEnable = VK_TRUE;

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