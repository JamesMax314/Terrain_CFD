#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

#include "geometry.hpp"

namespace MeshGen {
    void generateMesh(std::vector<Vertex>& vertices, std::vector<uint16_t>& indices, const std::string& filename, float gridSize, float maxHeight);
}
