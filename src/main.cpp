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

std::vector<float> init_density(size_t gridsize, float base_val) {
    std::vector<float> densities(gridsize * gridsize * gridsize);
    for (size_t i = 0; i < densities.size(); i += 1) {
        densities[i] = base_val;
    }
    return densities;
}

void print_vector(const std::vector<float>& vec) {
    for (const auto& val : vec) {
        std::cout << val << " ";
    }
    std::cout << "\n";
}

int main() {
    Init init;
    RenderData render_data;
    ComputeHandler compute_handler;


    if (0 != device_initialization(init)) return -1;
    if (0 != create_swapchain(init)) return -1;
    if (0 != get_queues(init, render_data)) return -1;
    if (0 != create_render_pass(init, render_data)) return -1;
    if (0 != create_graphics_pipeline(init, render_data)) return -1;
    if (0 != create_framebuffers(init, render_data)) return -1;
    if (0 != create_command_pool(init, render_data)) return -1;
    if (0 != create_command_buffers(init, render_data)) return -1;
    if (0 != create_sync_objects(init, render_data)) return -1;

    if (0 != get_comp_queue(init, compute_handler)) return -1;
    if (0 != create_command_pool(init, compute_handler)) return -1;

    // Later will need a different render pass to draw standard geometry

    const int gridSize = 10;
    const int bufferSize = gridSize * gridSize * gridSize * sizeof(float);
    buffer velocity = create_compute_buffer(init, bufferSize*3);
    buffer density = create_compute_buffer(init, bufferSize);
    buffer pressure = create_compute_buffer(init, bufferSize);
    buffer velocity2 = create_compute_buffer(init, bufferSize*3);
    buffer density2 = create_compute_buffer(init, bufferSize);
    buffer pressure2 = create_compute_buffer(init, bufferSize);
    std::vector<buffer> buffers = {velocity, density, pressure, velocity2, density2, pressure2};

    std::vector<float> velocities = init_velocities(gridSize, 0.0f, 1.0f, 0.0f);
    std::vector<float> densities = init_density(gridSize, 0.0f);
    densities[0] = 1.0f;

    copy_to_buffer(init, velocity, velocities.data());
    copy_to_buffer(init, density, densities.data());

    print_vector(densities);

    VkShaderModule shaderModule = createShaderModule(init, readFile(std::string(SHADER_DIR) + "/advect.spv"));
    kernel kern = build_kernal(init, compute_handler, shaderModule, buffers, bufferSize);
    // execute_kernel(init, compute_handler, kern);



    while (!glfwWindowShouldClose(init.window)) {
        glfwPollEvents();
        int res = draw_frame(init, render_data);
        if (res != 0) {
            std::cout << "failed to draw frame \n";
            return -1;
        }

        execute_kernel(init, compute_handler, kern);
        // copy_from_buffer(init, density2, densities.data());
        // copy_to_buffer(init, density, densities.data());
        // kern = build_kernal(init, compute_handler, shaderModule, buffers, bufferSize);
        // print_vector(densities);
    }
    init.disp.deviceWaitIdle();

    vkDestroyShaderModule(init.device.device, shaderModule, nullptr);
    cleanup(init, buffers);
    cleanup(init, kern);
    cleanup(init, compute_handler);
    cleanup(init, render_data);

    return 0;
}