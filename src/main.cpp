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

int main() {
    vulkan vk;
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

    // runCompute(vk.device, vk.physicalDevice, vk.graphicsQueue, vk.commandPool);

    const int N = 10;
    std::vector<float> input(N);
    std::vector<float> output(N);
    for (int i = 0; i < N; ++i) input[i] = float(i);

    // Create buffers and allocate memory
    VkDeviceSize bufferSize = sizeof(float) * N;

    buffer inBuffer = createComputeBuffer(vk, bufferSize);
    buffer outBuffer = createComputeBuffer(vk, bufferSize);
    std::vector<buffer> buffers = {inBuffer, outBuffer};

    copyToBuffer(vk, inBuffer, input.data());

    VkShaderModule shaderModule = createShaderModule(vk, readFile(std::string(SHADER_DIR) + "/example.spv"));

    kernel kern = buildKernal(vk, shaderModule, buffers, N);

    executeKernel(vk, kern);

    copyFromBuffer(vk, outBuffer, output.data());
    for (int i = 0; i < N; ++i)
        std::cout << input[i] << " * 2 = " << output[i] << "\n";
}