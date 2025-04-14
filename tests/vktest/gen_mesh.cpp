#include "gen_mesh.hpp"

std::vector<Vertex> generateGridVertices(const std::vector<std::vector<float>>& heightMap, float gridSize, float maxHeight) {
    std::vector<Vertex> vertices;

    size_t rows = heightMap.size();
    size_t cols = heightMap[0].size();

    for (size_t row = 0; row < rows; ++row) {
        for (size_t col = 0; col < cols; ++col) {
            float x = col * gridSize;
            float y = row * gridSize;
            float z = heightMap[row][col] * maxHeight;

            // Assign a color based on height (e.g., gradient from blue to green to red)
            glm::vec3 color = glm::vec3(z / maxHeight, 1.0f - z / maxHeight, 0.5f);

            vertices.push_back({{z, x, y}, color});
        }
    }

    return vertices;
}

std::vector<uint16_t> generateGridIndices(size_t rows, size_t cols) {
    std::vector<uint16_t> indices;

    for (size_t row = 0; row < rows - 1; ++row) {
        for (size_t col = 0; col < cols - 1; ++col) {
            uint16_t topLeft = row * cols + col;
            uint16_t topRight = topLeft + 1;
            uint16_t bottomLeft = (row + 1) * cols + col;
            uint16_t bottomRight = bottomLeft + 1;

            // First triangle
            indices.push_back(topLeft);
            indices.push_back(topRight);
            indices.push_back(bottomLeft);

            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomRight);
            indices.push_back(bottomLeft);
        }
    }

    return indices;
}

std::vector<std::vector<float>> readHeightMap(const std::string& filename) {
    std::vector<std::vector<float>> heightMap;
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream lineStream(line);
        std::vector<float> row;
        float value;

        while (lineStream >> value) {
            row.push_back(value);
        }

        heightMap.push_back(row);
    }

    file.close();
    return heightMap;
}

void MeshGen::generateMesh(std::vector<Vertex>& vertices, std::vector<uint16_t>& indices, const std::string& filename, float gridSize, float maxHeight) {
    std::vector<std::vector<float>> heightMap = readHeightMap(filename);
    vertices = generateGridVertices(heightMap, gridSize, maxHeight);
    indices = generateGridIndices(heightMap.size(), heightMap[0].size());
}