#include "cfd.hpp"

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
        uint x = i % gridsize;
        uint y = (i / gridsize) % gridsize;
        uint z = i / (gridsize * gridsize);
        scalars[i] = base_val;
        // if (sqrt(pow(x - (gridsize-1)/2, 2) + pow(y - (gridsize-1)/2, 2)) < 10) {
        //     scalars[i] = 0.0;
        // }
    }
    return scalars;
}

std::vector<float> init_cylinder(float base_val, int sizeX, int sizeY, int sizeZ, int radius) {
    std::vector<float> scalars(sizeX * sizeY * sizeZ);
    for (int i = 0; i < scalars.size(); i += 1) {
        int x = i % sizeX;
        int y = (i / sizeX) % sizeY;
        int z = i / (sizeX * sizeY);
        if (pow(x - (sizeX-1)/2, 2) + pow(y - (sizeY-1)/2, 2) < radius * radius) {
            scalars[i] = 0.0;
        } else {
            scalars[i] = base_val;
        }
    }
    return scalars;
}

std::vector<float> init_wall(float base_val, int sizeX, int sizeY, int sizeZ) {
    std::vector<float> scalars(sizeX * sizeY * sizeZ);
    for (int i = 0; i < scalars.size(); i += 1) {
        int x = i % sizeX;
        int y = (i / sizeX) % sizeY;
        int z = i / (sizeX * sizeY);
        if (x == 0 || x == sizeX-1) {
            scalars[i] = base_val;
        } else {
            scalars[i] = 0.0;
        }
    }
    return scalars;
}

std::vector<float> init_boundaries(int gridSize) {
    std::vector<float> scalars(gridSize * gridSize * gridSize);
    for (int i = 0; i < scalars.size(); i += 1) {
        int x = i % gridSize;
        int y = (i / gridSize) % gridSize;
        int z = i / (gridSize * gridSize);
        if (x % (gridSize) == 0 || y % (gridSize) == 0 || z % (gridSize) == 0) {
            scalars[i] = 0.0;
        } else {
            scalars[i] = 1.0;
        }
    }
    return scalars;
}

void add_boundary_cylinder(std::vector<float>& boundaries, int rad, int posX, int posY, int gridsize) {
    for (int i = 0; i < boundaries.size(); i += 1) {
        int x = i % gridsize;
        int y = (i / gridsize) % gridsize;
        int z = i / (gridsize * gridsize);

        if (pow(x-1-posX - (gridsize-1)/2, 2) + pow(y-1-posY - (gridsize-1)/2, 2) < rad*rad) {
            boundaries[i] = 0.0;
        }
    }
}

kernel build_compute_kernal(Init& init, ComputeHandler& handler, VkShaderModule& shaderModule, std::vector<buffer>& buffers, std::vector<texture>& textures, PushConstants& pushConsts, size_t nThreads) {
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

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(pushConsts);

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

    vkCmdPushConstants(
        kern.cmdBuf,
        kern.pipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(pushConsts),
        &pushConsts
    );

    vkCmdDispatch(kern.cmdBuf, nThreads, 1, 1);
    vkEndCommandBuffer(kern.cmdBuf);

    return kern;
}


kernel gaussSiedelKernel(Init& init, ComputeHandler& handler, VkShaderModule& shaderModule, std::vector<buffer>& buffers, PushConstants& pushConsts, size_t nThreads) {
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
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(pushConsts);

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

    pushConsts.shouldRed = 1;

    vkCmdPushConstants(
        kern.cmdBuf,
        kern.pipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(pushConsts),
        &pushConsts
    );

    vkCmdBindPipeline(kern.cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, kern.pipeline);
    vkCmdBindDescriptorSets(kern.cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, kern.pipelineLayout, 0, 1, &kern.descriptorSet, 0, nullptr);

    vkCmdDispatch(kern.cmdBuf, nThreads, 1, 1);

    pushConsts. shouldRed = 0;

    vkCmdPushConstants(
        kern.cmdBuf,
        kern.pipelineLayout,
        VK_SHADER_STAGE_COMPUTE_BIT,
        0,
        sizeof(pushConsts),
        &pushConsts
    );

    vkCmdDispatch(kern.cmdBuf, nThreads, 1, 1);
    vkEndCommandBuffer(kern.cmdBuf);

    return kern;
}

void init_cfd(Init& init, ComputeHandler& computeHandler, Cfd& cfd, int gridSize) {
    cfd.gridSize = gridSize;
    const uint local_work_size = 32;

    const int bufferSize = gridSize * gridSize * gridSize * sizeof(float);
    const int velBufferSize = (gridSize+1) * gridSize * gridSize * sizeof(float);
    const int boarderBufferSize = (gridSize+2) * (gridSize+2) * (gridSize+2) * sizeof(float);
    const int nThreads = (gridSize * gridSize * gridSize + local_work_size - 1) / local_work_size;
    const int nThreadsVel = ((gridSize+1) * gridSize * gridSize + local_work_size - 1) / local_work_size;


    cfd.boundaries = create_compute_buffer(init, boarderBufferSize);

    cfd.vx = create_compute_buffer(init, velBufferSize);
    cfd.vy = create_compute_buffer(init, velBufferSize);
    cfd.vz = create_compute_buffer(init, velBufferSize);

    cfd.vx2 = create_compute_buffer(init, velBufferSize);
    cfd.vy2 = create_compute_buffer(init, velBufferSize);
    cfd.vz2 = create_compute_buffer(init, velBufferSize);

    cfd.density = create_compute_buffer(init, bufferSize);
    cfd.pressure = create_compute_buffer(init, bufferSize);

    cfd.density2 = create_compute_buffer(init, bufferSize);
    cfd.pressure2 = create_compute_buffer(init, bufferSize);


    cfd.densityTex.x = gridSize;
    cfd.densityTex.y = gridSize;
    cfd.densityTex.z = gridSize;
    create3DTexture(init, cfd.densityTex);
    std::vector<texture> textures = {cfd.densityTex};


    PushConstants pushConsts;
    pushConsts.gridSize = gridSize;


    std::vector<buffer> buffersGaussSiedel = {cfd.vx, cfd.vy, cfd.vz, cfd.boundaries};
    VkShaderModule shaderGaussSiedel = createShaderModule(init, readFile(std::string(SHADER_DIR) + "/gaussSiedel.spv"));
    cfd.kernGaussSiedel = gaussSiedelKernel(init, computeHandler, shaderGaussSiedel, buffersGaussSiedel, pushConsts, nThreads);

    VkShaderModule shaderModule = createShaderModule(init, readFile(std::string(SHADER_DIR) + "/advect.spv"));
    std::vector<buffer> buffers = {cfd.vx, cfd.vy, cfd.vz, cfd.density, cfd.pressure, cfd.vx2, cfd.vy2, cfd.vz2, cfd.density2, cfd.pressure2, cfd.boundaries};
    std::vector<buffer> buffers2 = {cfd.vx2, cfd.vy2, cfd.vz2, cfd.density2, cfd.pressure2, cfd.vx, cfd.vy, cfd.vz, cfd.density, cfd.pressure, cfd.boundaries};
    cfd.kern = build_compute_kernal(init, computeHandler, shaderModule, buffers, textures, pushConsts, nThreadsVel);
    cfd.kern2 = build_compute_kernal(init, computeHandler, shaderModule, buffers2, textures, pushConsts, nThreadsVel);

    VkShaderModule shaderModuleWrtieTex = createShaderModule(init, readFile(std::string(SHADER_DIR) + "/writeTexture.spv"));
    std::vector<buffer> buffersWriteTex = {cfd.vx, cfd.vy, cfd.vz, cfd.density, cfd.pressure, cfd.density2, cfd.pressure2, cfd.boundaries};
    std::vector<buffer> buffersWriteTex2 = {cfd.vx, cfd.vy, cfd.vz, cfd.density2, cfd.pressure2, cfd.density, cfd.pressure, cfd.boundaries};
    cfd.kernWriteTex = build_compute_kernal(init, computeHandler, shaderModuleWrtieTex, buffersWriteTex, textures, pushConsts, nThreads);
    cfd.kernWriteTex2 = build_compute_kernal(init, computeHandler, shaderModuleWrtieTex, buffersWriteTex2, textures, pushConsts, nThreads);

    // Wall of x flow
    std::vector<float> vxs = init_wall(2.0f, gridSize+1, gridSize, gridSize);
    std::vector<float> vys = init_vels(gridSize, 0.0f);
    std::vector<float> vzs = init_vels(gridSize, 0.0f);
    std::vector<float> densities = init_scalars(gridSize, 0.0f);
    std::vector<float> boundariesVec = init_boundaries(gridSize+2);

    // Arbitrary Geometry
    // add_boundary_cylinder(boundariesVec, 10, 0, 0, gridSize+2);
    // add_boundary_cylinder(boundariesVec, 10, -20, -20, gridSize+2);

    int nStreams = 10;
    int streamSize = gridSize / nStreams;
    for (int i=0; i<nStreams; i++) {
        densities[gridSize*gridSize*(gridSize/2) + gridSize*i*streamSize + 0] = 2.0f;
    }

    for (int i=0; i<gridSize+2; i++)
    {
        boundariesVec[(gridSize+2)*(gridSize+2)*(gridSize/2+1) + (gridSize+2)*(i) + (0+1)] = 0.0f;
    }

    copy_to_buffer(init, cfd.vx, vxs.data());
    copy_to_buffer(init, cfd.vy, vys.data());
    copy_to_buffer(init, cfd.vz, vzs.data());
    copy_to_buffer(init, cfd.density, densities.data());
    copy_to_buffer(init, cfd.boundaries, boundariesVec.data());

    init.disp.destroyShaderModule(shaderGaussSiedel, nullptr);
    init.disp.destroyShaderModule(shaderModule, nullptr);
    init.disp.destroyShaderModule(shaderModuleWrtieTex, nullptr);
}

void loadTerrain(const std::string& filename, std::vector<float>& terrain, int& sizeX, int& sizeY) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    sizeX = 0;
    terrain.clear();

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        float value;
        sizeY = 0;

        while (iss >> value) {
            terrain.push_back(value);
            sizeY += 1;
        }
        sizeX += 1;
    }
    file.close();
}

void load_terrain(Init& init, Cfd& cfd, const std::string& filename) {
    int terrainSizeX, terrainSizeY;
    std::vector<float> terrain;
    loadTerrain(filename, terrain, terrainSizeX, terrainSizeY);

    std::cout << "Terrain size: " << terrainSizeX << " x " << terrainSizeY << std::endl;

    int gridSize = cfd.gridSize;
    int boundarySize = gridSize + 2;
    std::vector<float> boundariesVec = init_boundaries(boundarySize);
    
    float terrainStepX = terrainSizeX / float(gridSize);
    float terrainStepY = terrainSizeY / float(gridSize);

    std::cout << "Terrain step: " << terrainStepX << " x " << terrainStepY << std::endl;

    for (int i = 0; i < boundariesVec.size(); i += 1) {
        int x = i % boundarySize;
        int y = (i / boundarySize) % boundarySize;
        int z = i / (boundarySize * boundarySize);

        if (x > 0 && x < gridSize+1 && y > 0 && y < gridSize+1) {
            int terrainX = (x-1) * terrainStepX;
            int terrainY = (y-1) * terrainStepY;
    
            float terrainHeight = terrain[terrainX + terrainY*terrainSizeX];

            if (z >= terrainHeight*boundarySize) {
                boundariesVec[i] = 1.0;
            } else {
                boundariesVec[i] = 0.0;
            }
        }
    }

    for (int i=0; i<gridSize+2; i++)
    {
        boundariesVec[(gridSize+2)*(gridSize+2)*(gridSize/2+1) + (gridSize+2)*(i) + (0+1)] = 0.0f;
    }

    copy_to_buffer(init, cfd.boundaries, boundariesVec.data());
}

void evolve_cfd(Init& init, ComputeHandler& computeHandler, Cfd& cfd) {
    for (int i=0; i<10; i++)
    {
        execute_kernel(init, computeHandler, cfd.kernGaussSiedel);
    }

    execute_kernel(init, computeHandler, cfd.kern);
    execute_kernel(init, computeHandler, cfd.kern2);

    execute_kernel(init, computeHandler, cfd.kernWriteTex);
    execute_kernel(init, computeHandler, cfd.kernWriteTex2);
}

void cleanup(Init &init, Cfd &cfd)
{
    cleanup(init, cfd.kernGaussSiedel);
    cleanup(init, cfd.kern);
    cleanup(init, cfd.kern2);
    cleanup(init, cfd.kernWriteTex);
    cleanup(init, cfd.kernWriteTex2);

    std::vector<buffer> buffers = {cfd.vx, cfd.vy, cfd.vz, cfd.density, cfd.pressure, cfd.vx2, cfd.vy2, cfd.vz2, cfd.density2, cfd.pressure2, cfd.boundaries};
    cleanup(init, buffers);
    cleanup(init, cfd.densityTex);   
}
