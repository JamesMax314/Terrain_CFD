#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vkHelper.hpp"
#include "shaderHelper.hpp"
#include "renderHelper.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}
};

std::vector<uint32_t> indices = {
    0, 1, 2, 2, 3, 0
};

int main() {
    vulkan vk;
    renderer ren;
    VkCommandBuffer commandBuffer;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    setDebugMode(vk, enableValidationLayers);
    createInstance(vk, "Compute Test", "Custom Engine");
    setupDebugMessenger(vk);
    createSurface(vk, window);
    pickPhysicalDevice(vk);
    createLogicalDevice(vk);
    createCommandPool(vk);
    
    // Setup the renderer
    createSwapChain(vk, window, ren);
    createImageViews(vk, ren);
    createDepthResources(vk, ren);
    createRenderPass(vk, ren);
    createDescriptorSetLayout(vk, ren);
    createGraphicsPipeline(vk, ren);
    createFramebuffers(vk, ren);
    setMesh(vk, ren, vertices, indices);
    createUniformBuffers(vk, ren);
    createDescriptorPool(vk, ren);
    createDescriptorSets(vk, ren);
    createCommandBuffers(vk, ren);
    createSyncObjects(vk, ren);

    std::cout << ren.vertices[0].pos.x << " " << ren.vertices[0].pos.y << " " << ren.vertices[0].pos.z << "\n";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // processInput();
        drawFrame(vk, ren, window);
    }

    vkDeviceWaitIdle(vk.device);

    // const int N = 10;
    // std::vector<float> input(N);
    // std::vector<float> output(N);
    // for (int i = 0; i < N; ++i) input[i] = float(i);

    // // Create buffers and allocate memory
    // VkDeviceSize bufferSize = sizeof(float) * N;

    // buffer inBuffer = createComputeBuffer(vk, bufferSize);
    // buffer outBuffer = createComputeBuffer(vk, bufferSize);
    // std::vector<buffer> buffers = {inBuffer, outBuffer};

    // copyToBuffer(vk, inBuffer, input.data());

    // VkShaderModule shaderModule = createShaderModule(vk, readFile(std::string(SHADER_DIR) + "/example.spv"));

    // kernel kern = buildKernal(vk, shaderModule, buffers, N);

    // executeKernel(vk, kern);

    // copyFromBuffer(vk, outBuffer, output.data());
    // for (int i = 0; i < N; ++i)
    //     std::cout << input[i] << " * 2 = " << output[i] << "\n";


    const int gridSize = 10;
    const int bufferSize = gridSize * gridSize * gridSize * sizeof(float);
    buffer velocity = createComputeBuffer(vk, bufferSize*3);
    buffer density = createComputeBuffer(vk, bufferSize);
    buffer pressure = createComputeBuffer(vk, bufferSize);
    buffer velocity2 = createComputeBuffer(vk, bufferSize*3);
    buffer density2 = createComputeBuffer(vk, bufferSize);
    buffer pressure2 = createComputeBuffer(vk, bufferSize);
    std::vector<buffer> buffers = {velocity, density, pressure, velocity2, density2, pressure2};

    VkShaderModule shaderModule = createShaderModule(vk, readFile(std::string(SHADER_DIR) + "/advect.spv"));

    kernel kern = buildKernal(vk, shaderModule, buffers, bufferSize);

    executeKernel(vk, kern);

    
}