#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "vkHelper.hpp"
#include "shaderHelper.hpp"
#include "cfd.hpp"
#include "plainRenderer.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;
const uint local_work_size = 32;

const std::string heightFile = "/Users/jamesmaxwell/Documents/Projects/Terrain_CFD/Data/out_data.txt";

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

int main() {
    Init init;
    RenderData render_data;
    ComputeHandler compute_handler;
    Cfd cfd;

    const int gridSize = 129;

    if (0 != device_initialization(init)) return -1;
    if (0 != create_swapchain(init)) return -1;
    if (0 != get_queues(init, render_data)) return -1;
    if (0 != create_render_pass(init, render_data)) return -1;

    if (0 != get_comp_queue(init, compute_handler)) return -1;
    if (0 != create_command_pool(init, compute_handler)) return -1;

    // Later will need a different render pass to draw standard geometry

    init_cfd(init, compute_handler, cfd, gridSize);
    load_terrain(init, cfd, heightFile);

    std::vector<texture> textures = {cfd.densityTex};
    
    if (0 != create_graphics_pipeline(init, render_data, textures)) return -1;
    if (0 != create_framebuffers(init, render_data)) return -1;
    if (0 != create_command_pool(init, render_data)) return -1;
    if (0 != create_command_buffers(init, render_data, textures)) return -1;
    if (0 != create_sync_objects(init, render_data)) return -1;

    // execute_kernel(init, compute_handler, kern);

    bool toggle = false;
    while (!glfwWindowShouldClose(init.window)) {
        glfwPollEvents();
        int res = draw_frame(init, render_data);
        if (res != 0) {
            std::cout << "failed to draw frame \n";
            return -1;
        }

        evolve_cfd(init, compute_handler, cfd);

    }
    init.disp.deviceWaitIdle();

    cleanup(init, cfd);
    cleanup(init, compute_handler);
    cleanup(init, render_data);

    return 0;
}